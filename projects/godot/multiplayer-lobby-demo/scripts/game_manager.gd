extends Node3D

# GameManager - Generic rendering layer for shadowblox games with lobby support
# This layer is game-agnostic - Luau controls all game-specific logic
# GDScript only handles: rendering, input forwarding, network transport
# Now uses NodeTunnel with host migration support

var sbx_runtime = null
var sbx_available := false

# UI References
@onready var ui_layer: CanvasLayer = $UILayer
@onready var status_label: Label = %StatusLabel
@onready var player_list: ItemList = %PlayerList
@onready var main_menu: Control = $UILayer/MainMenu
@onready var game_ui: Control = $UILayer/GameUI

# Main menu UI
@onready var host_button: Button = %HostButton
@onready var join_button: Button = %JoinButton
@onready var browse_button: Button = %BrowseButton
@onready var host_id_input: LineEdit = %HostIdInput
@onready var name_input: LineEdit = %NameInput
@onready var lobby_name_input: LineEdit = %LobbyNameInput
@onready var online_id_label: Label = %OnlineIdLabel

# Lobby browser UI
@onready var lobby_list_container: Control = %LobbyListContainer
@onready var lobby_list_placeholder: Control = %LobbyListPlaceholder

# Game UI
@onready var leave_button: Button = %LeaveButton
@onready var migration_label: Label = %MigrationLabel

# Visual tracking
var local_player_node: Node3D = null
var player_nodes := {}  # user_id -> Node3D
var player_materials := {}  # user_id -> StandardMaterial3D

# Host migration state
var migration_in_progress := false
var players_before_migration := {}


func _ready() -> void:
	# Connect NodeTunnelManager signals
	NodeTunnelManager.relay_connected.connect(_on_relay_connected)
	NodeTunnelManager.hosting_started.connect(_on_hosting_started)
	NodeTunnelManager.joined_lobby.connect(_on_joined_lobby)
	NodeTunnelManager.registration_complete.connect(_on_registration_complete)
	NodeTunnelManager.left_room.connect(_on_left_room)
	NodeTunnelManager.player_joined.connect(_on_player_joined)
	NodeTunnelManager.player_left.connect(_on_player_left)
	NodeTunnelManager.existing_player.connect(_on_existing_player)
	NodeTunnelManager.world_state_received.connect(_on_world_state_received)
	NodeTunnelManager.host_migration_started.connect(_on_host_migration_started)
	NodeTunnelManager.host_migration_completed.connect(_on_host_migration_completed)
	NodeTunnelManager.host_migration_failed.connect(_on_host_migration_failed)

	# Connect UI buttons
	host_button.pressed.connect(_on_host_pressed)
	join_button.pressed.connect(_on_join_pressed)
	browse_button.pressed.connect(_on_browse_pressed)
	leave_button.pressed.connect(_on_leave_pressed)

	# Initially show main menu
	main_menu.visible = true
	game_ui.visible = false
	lobby_list_container.visible = false
	migration_label.visible = false

	# Disable buttons until relay connected
	host_button.disabled = true
	join_button.disabled = true
	browse_button.disabled = true
	online_id_label.text = "Connecting..."

	# Initialize SbxRuntime
	if ClassDB.class_exists("SbxRuntime"):
		sbx_available = true
		sbx_runtime = ClassDB.instantiate("SbxRuntime")
		if sbx_runtime:
			sbx_runtime.name = "SbxRuntime"
			add_child(sbx_runtime)
			print("[GameManager] ShadowBlox extension loaded")

			# Connect to generic rendering signals from Luau
			sbx_runtime.player_color_changed.connect(_on_player_color_changed)
			sbx_runtime.status_text_changed.connect(_on_status_text_changed)

			# Wait a frame for initialization
			await get_tree().process_frame
			_setup_luau_game()
		else:
			print("[GameManager] Failed to instantiate SbxRuntime")
			sbx_available = false
	else:
		print("[GameManager] ShadowBlox extension not available")


func _setup_luau_game() -> void:
	# Load game_init.luau first (sets up workspace, ReplicatedStorage)
	var init_result = _run_luau_script("res://luau/game_init.luau")
	if init_result != "OK":
		printerr("[GameManager] game_init.luau failed: ", init_result)
		return

	# Load lobby_manager.luau (lobby state management)
	var lobby_result = _run_luau_script("res://luau/lobby_manager.luau")
	if lobby_result != "OK":
		printerr("[GameManager] lobby_manager.luau failed: ", lobby_result)
		return

	# Then load game_manager.luau (game logic)
	var manager_result = _run_luau_script("res://luau/game_manager.luau")
	if manager_result != "OK":
		printerr("[GameManager] game_manager.luau failed: ", manager_result)
		return

	print("[GameManager] Luau scripts loaded successfully")


func _run_luau_script(path: String) -> String:
	var file = FileAccess.open(path, FileAccess.READ)
	if not file:
		return "File not found: " + path
	var code = file.get_as_text()
	file.close()
	return sbx_runtime.execute_script(code)


# ============================================================================
# NETWORK EVENT HANDLERS
# ============================================================================

func _on_relay_connected(online_id: String) -> void:
	print("[GameManager] Relay connected. ID: ", online_id)
	online_id_label.text = "Online ID: " + online_id + " [" + NodeTunnelManager.get_topology_name() + "]"
	host_button.disabled = false
	join_button.disabled = false
	browse_button.disabled = false


func _on_hosting_started() -> void:
	print("[GameManager] Started hosting")

	main_menu.visible = false
	game_ui.visible = true
	lobby_list_container.visible = false

	var display_name = name_input.text if name_input.text != "" else "Host"

	if sbx_runtime and sbx_available:
		sbx_runtime.set_is_server(true)
		sbx_runtime.create_player(1, display_name)
		sbx_runtime.load_character(1)

	_spawn_player_node(1, display_name, true)
	status_label.text = "Hosting - " + NodeTunnelManager.get_topology_name()


func _on_joined_lobby(host_id: String) -> void:
	print("[GameManager] Joined lobby: ", host_id)

	main_menu.visible = false
	game_ui.visible = true
	lobby_list_container.visible = false

	if sbx_runtime and sbx_available:
		sbx_runtime.set_is_client(true)

	status_label.text = "Connected to " + host_id


func _on_registration_complete(user_id: int) -> void:
	# Called when the server has assigned us a user_id
	# Now we can create our local player with the correct ID
	print("[GameManager] Registration complete. User ID: ", user_id)

	var display_name = name_input.text if name_input.text != "" else "Player"

	if sbx_runtime and sbx_available:
		sbx_runtime.create_local_player(user_id, display_name)
		sbx_runtime.load_character(user_id)

	_spawn_player_node(user_id, display_name, true)
	_update_player_list()


func _on_left_room() -> void:
	print("[GameManager] Left room")
	_cleanup_game()
	main_menu.visible = true
	game_ui.visible = false
	lobby_list_container.visible = false
	migration_label.visible = false


func _on_player_joined(user_id: int, display_name: String, _online_id: String) -> void:
	print("[GameManager] Player joined: ", display_name, " (ID: ", user_id, ")")

	# Create player in Luau
	if sbx_runtime and sbx_available:
		sbx_runtime.create_player(user_id, display_name)
		sbx_runtime.load_character(user_id)

	if not player_nodes.has(user_id):
		var is_local = (user_id == NodeTunnelManager.local_player_id)
		_spawn_player_node(user_id, display_name, is_local)

	_update_player_list()


func _on_player_left(user_id: int, _online_id: String) -> void:
	print("[GameManager] Player left: ", user_id)

	# Skip during migration
	if migration_in_progress:
		print("[GameManager] Skipping removal during migration")
		return

	if NodeTunnelManager.is_server and sbx_runtime and sbx_available:
		sbx_runtime.remove_player(user_id)

	if player_nodes.has(user_id):
		player_nodes[user_id].queue_free()
		player_nodes.erase(user_id)

	if player_materials.has(user_id):
		player_materials.erase(user_id)

	_update_player_list()


func _on_existing_player(user_id: int, display_name: String, initial_pos: Vector3) -> void:
	print("[GameManager] Existing player: ", display_name, " (ID: ", user_id, ")")

	# Create existing players in Luau
	if sbx_runtime and sbx_available:
		sbx_runtime.create_player(user_id, display_name)
		sbx_runtime.load_character(user_id)
		if initial_pos != Vector3.ZERO:
			sbx_runtime.set_player_position(user_id, initial_pos)

	if not player_nodes.has(user_id):
		_spawn_player_node(user_id, display_name, false, initial_pos)
	_update_player_list()


func _on_world_state_received(state: Dictionary) -> void:
	print("[GameManager] World state received")
	# Players are handled via existing_player signals
	# Game state sync could be added here if needed


# ============================================================================
# HOST MIGRATION HANDLERS
# ============================================================================

func _on_host_migration_started(reason: String) -> void:
	print("[GameManager] Host migration started: ", reason)
	migration_in_progress = true
	migration_label.text = "Host migration in progress..."
	migration_label.visible = true

	# Backup player states
	players_before_migration = player_nodes.duplicate()

	# Notify Luau
	if sbx_runtime and sbx_available:
		# Could call Luau function here if exposed
		pass


func _on_host_migration_completed(new_host_id: String) -> void:
	print("[GameManager] Host migration completed. New host: ", new_host_id)
	migration_in_progress = false
	migration_label.visible = false

	# Wait for state to settle
	await get_tree().process_frame
	await get_tree().process_frame

	# Check if we became the new host
	if multiplayer.is_server():
		print("[GameManager] We are now the host!")
		status_label.text = "Hosting (migrated) - " + NodeTunnelManager.get_topology_name()

		if sbx_runtime and sbx_available:
			sbx_runtime.set_is_server(true)

		# Handle player node renaming if our ID changed
		_handle_post_migration_cleanup()
	else:
		status_label.text = "Connected (migrated)"

	players_before_migration.clear()
	_update_player_list()


func _on_host_migration_failed(reason: String) -> void:
	print("[GameManager] Host migration failed: ", reason)
	migration_in_progress = false
	migration_label.text = "Migration failed: " + reason
	await get_tree().create_timer(3.0).timeout
	migration_label.visible = false
	players_before_migration.clear()


func _handle_post_migration_cleanup() -> void:
	# After becoming the new host, our user_id changes from N to 1
	# Need to update player node references

	var our_old_node = null
	var our_old_id = -1

	# Find our old player node (the one we controlled before migration)
	for user_id in player_nodes:
		if user_id != 1 and player_nodes.has(user_id):
			if player_nodes[user_id] == local_player_node:
				our_old_node = player_nodes[user_id]
				our_old_id = user_id
				break

	if our_old_node and our_old_id != -1:
		print("[GameManager] Renaming our player node from ", our_old_id, " to 1")
		player_nodes.erase(our_old_id)
		player_nodes[1] = our_old_node
		our_old_node.name = "Player_1"

		if player_materials.has(our_old_id):
			player_materials[1] = player_materials[our_old_id]
			player_materials.erase(our_old_id)

	# Remove the old host's player node (ID 1 from before)
	# This should have been the previous host who disconnected
	# The peer disconnect should handle this normally


# ============================================================================
# UI HANDLERS
# ============================================================================

func _on_host_pressed() -> void:
	var display_name = name_input.text if name_input.text != "" else "Host"
	var lobby_name = lobby_name_input.text if lobby_name_input.text != "" else "My Lobby"

	host_button.disabled = true
	join_button.disabled = true
	browse_button.disabled = true
	host_button.text = "Hosting..."

	NodeTunnelManager.host_game(lobby_name, display_name)

	host_button.text = "Host Game"
	# Buttons stay disabled while in game


func _on_join_pressed() -> void:
	var host_id = host_id_input.text
	if host_id.is_empty():
		status_label.text = "Enter a Host ID to join"
		return

	var display_name = name_input.text if name_input.text != "" else "Player"

	host_button.disabled = true
	join_button.disabled = true
	browse_button.disabled = true
	join_button.text = "Joining..."

	# This will connect to the lobby and register with the server
	# When registration completes, _on_registration_complete will be called
	# with our assigned user_id, and we'll create the local player there
	NodeTunnelManager.join_game(host_id, display_name)

	join_button.text = "Join Game"
	# Local player creation happens in _on_registration_complete


func _on_browse_pressed() -> void:
	lobby_list_container.visible = true
	_load_lobby_list()


func _on_leave_pressed() -> void:
	NodeTunnelManager.leave_game()


func _on_close_lobby_list_pressed() -> void:
	lobby_list_container.visible = false


func _load_lobby_list() -> void:
	if lobby_list_placeholder.get_child_count() == 0:
		var lobby_list_scene = preload("res://addons/nodetunnel/new_demo/lobby/lobby_list.tscn")
		var lobby_list = lobby_list_scene.instantiate()
		lobby_list_placeholder.add_child(lobby_list)
		lobby_list.lobby_selected.connect(_on_lobby_selected)
		lobby_list.anchors_preset = Control.PRESET_FULL_RECT
	else:
		var lobby_list = lobby_list_placeholder.get_child(0)
		lobby_list.refresh_lobbies()


func _on_lobby_selected(lobby_id: String) -> void:
	lobby_list_container.visible = false
	host_id_input.text = lobby_id
	_on_join_pressed()


# ============================================================================
# RENDERING (GDScript's core responsibility)
# ============================================================================

func _on_player_color_changed(user_id: int, color: Color) -> void:
	if player_materials.has(user_id):
		player_materials[user_id].albedo_color = color

	# Sync color to clients via RPC
	if NodeTunnelManager.is_server:
		_sync_player_color.rpc(user_id, color)


func _on_status_text_changed(text: String) -> void:
	status_label.text = text

	# Sync status to clients via RPC
	if NodeTunnelManager.is_server:
		_sync_status_text.rpc(text)


func _spawn_player_node(user_id: int, display_name: String, is_local: bool, initial_position: Vector3 = Vector3.ZERO) -> void:
	var player_node = Node3D.new()
	player_node.name = "Player_" + str(user_id)

	# Create body mesh
	var body = MeshInstance3D.new()
	var capsule = CapsuleMesh.new()
	capsule.radius = 0.5
	capsule.height = 2.0
	body.mesh = capsule
	body.name = "Body"

	# Create material
	var material = StandardMaterial3D.new()
	material.albedo_color = Color.GREEN if is_local else Color.BLUE
	body.material_override = material
	player_materials[user_id] = material

	player_node.add_child(body)

	# Add name label
	var label_3d = Label3D.new()
	label_3d.text = display_name
	label_3d.position.y = 1.5
	label_3d.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	player_node.add_child(label_3d)

	# Initial position
	if initial_position != Vector3.ZERO:
		player_node.position = initial_position
	else:
		player_node.position = Vector3(randf_range(-5, 5), 1, randf_range(-5, 5))

	add_child(player_node)
	player_nodes[user_id] = player_node

	# Add camera for local player
	if is_local:
		local_player_node = player_node
		var camera = Camera3D.new()
		camera.position = Vector3(0, 5, 10)
		player_node.add_child(camera)
		camera.look_at(Vector3.ZERO)
		camera.current = true


func _process(_delta: float) -> void:
	if not sbx_runtime or not sbx_available:
		return

	# Forward input to Luau
	if local_player_node:
		_forward_input_to_luau()

	# Read positions from Luau and update meshes
	_sync_positions_from_luau()

	# Send position updates over network
	_send_position_update()


func _forward_input_to_luau() -> void:
	var direction = Vector3.ZERO

	if Input.is_action_pressed("move_forward"):
		direction.z -= 1
	if Input.is_action_pressed("move_backward"):
		direction.z += 1
	if Input.is_action_pressed("move_left"):
		direction.x -= 1
	if Input.is_action_pressed("move_right"):
		direction.x += 1

	if direction != Vector3.ZERO:
		direction = direction.normalized()

	sbx_runtime.set_input_direction(NodeTunnelManager.local_player_id, direction)


func _sync_positions_from_luau() -> void:
	var positions: Dictionary = sbx_runtime.get_all_player_positions()

	for user_id in positions:
		var pos: Vector3 = positions[user_id]
		if player_nodes.has(user_id):
			player_nodes[user_id].position = pos


var _last_sent_position := Vector3.ZERO

func _send_position_update() -> void:
	if not local_player_node:
		return

	var current_pos = local_player_node.position
	if current_pos.distance_to(_last_sent_position) < 0.01:
		return

	_last_sent_position = current_pos
	var user_id = NodeTunnelManager.local_player_id

	if NodeTunnelManager.is_server:
		_broadcast_position.rpc(user_id, current_pos)
	else:
		_send_position_to_server.rpc_id(1, user_id, current_pos)


@rpc("any_peer", "unreliable_ordered")
func _send_position_to_server(user_id: int, pos: Vector3) -> void:
	if not NodeTunnelManager.is_server:
		return

	if sbx_runtime and sbx_available:
		sbx_runtime.set_player_position(user_id, pos)

	_broadcast_position.rpc(user_id, pos)


@rpc("authority", "unreliable_ordered", "call_local")
func _broadcast_position(user_id: int, pos: Vector3) -> void:
	if sbx_runtime and sbx_available and user_id != NodeTunnelManager.local_player_id:
		sbx_runtime.set_player_position(user_id, pos)


@rpc("authority", "reliable", "call_local")
func _sync_player_color(user_id: int, color: Color) -> void:
	if player_materials.has(user_id):
		player_materials[user_id].albedo_color = color


@rpc("authority", "reliable", "call_local")
func _sync_status_text(text: String) -> void:
	status_label.text = text


func _update_player_list() -> void:
	player_list.clear()
	for user_id in player_nodes:
		var name_text = "Player " + str(user_id)
		if user_id == NodeTunnelManager.local_player_id:
			name_text += " (You)"
		if NodeTunnelManager.is_server and user_id == 1:
			name_text += " [Host]"
		player_list.add_item(name_text)


func _cleanup_game() -> void:
	for user_id in player_nodes:
		player_nodes[user_id].queue_free()
	player_nodes.clear()
	player_materials.clear()
	local_player_node = null
	players_before_migration.clear()
	migration_in_progress = false
