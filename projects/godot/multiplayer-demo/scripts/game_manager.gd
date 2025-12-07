extends Node3D

# GameManager - Generic rendering layer for shadowblox games
# This layer is game-agnostic - Luau controls all game-specific logic
# GDScript only handles: rendering, input forwarding, network transport

var sbx_runtime = null
var sbx_available := false

@onready var ui_layer: CanvasLayer = $UILayer
@onready var status_label: Label = %StatusLabel
@onready var player_list: ItemList = %PlayerList
@onready var host_button: Button = %HostButton
@onready var join_button: Button = %JoinButton
@onready var address_input: LineEdit = %AddressInput
@onready var name_input: LineEdit = %NameInput
@onready var main_menu: Control = $UILayer/MainMenu
@onready var game_ui: Control = $UILayer/GameUI

var local_player_node: Node3D = null
var player_nodes := {}  # user_id -> Node3D
var player_materials := {}  # user_id -> StandardMaterial3D

func _ready() -> void:
	# Connect network manager signals
	NetworkManager.server_started.connect(_on_server_started)
	NetworkManager.client_connected.connect(_on_client_connected)
	NetworkManager.client_disconnected.connect(_on_client_disconnected)
	NetworkManager.player_joined.connect(_on_player_joined)
	NetworkManager.player_left.connect(_on_player_left)
	NetworkManager.existing_player.connect(_on_existing_player)

	# Connect UI buttons
	host_button.pressed.connect(_on_host_pressed)
	join_button.pressed.connect(_on_join_pressed)

	# Initially show main menu
	main_menu.visible = true
	game_ui.visible = false

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

func _on_host_pressed() -> void:
	var display_name = name_input.text if name_input.text != "" else "Host"
	var error = NetworkManager.start_server(7777, display_name)
	if error == OK:
		main_menu.visible = false
		game_ui.visible = true

		if sbx_runtime and sbx_available:
			sbx_runtime.set_is_server(true)
			sbx_runtime.create_player(1, display_name)
			sbx_runtime.load_character(1)

		_spawn_player_node(1, display_name, true)
		status_label.text = "Hosting..."

func _on_join_pressed() -> void:
	var address = address_input.text if address_input.text != "" else "127.0.0.1"
	var display_name = name_input.text if name_input.text != "" else "Player"

	var error = NetworkManager.join_server(address, 7777, display_name)
	if error == OK:
		main_menu.visible = false
		game_ui.visible = true

		if sbx_runtime and sbx_available:
			sbx_runtime.set_is_client(true)

		status_label.text = "Connecting..."

func _on_server_started() -> void:
	print("[GameManager] Server started")

func _on_client_connected() -> void:
	print("[GameManager] Connected to server")

	var user_id = NetworkManager.local_player_id
	var display_name = name_input.text if name_input.text != "" else "Player"
	if sbx_runtime and sbx_available:
		sbx_runtime.create_local_player(user_id, display_name)
		sbx_runtime.load_character(user_id)
	_spawn_player_node(user_id, display_name, true)

func _on_client_disconnected() -> void:
	print("[GameManager] Disconnected")
	_cleanup_game()
	main_menu.visible = true
	game_ui.visible = false

func _on_player_joined(user_id: int, display_name: String) -> void:
	print("[GameManager] Player joined: ", display_name, " (ID: ", user_id, ")")

	# Create player in Luau (both server and client need this for position sync)
	if sbx_runtime and sbx_available:
		if NetworkManager.is_server:
			sbx_runtime.create_player(user_id, display_name)
			sbx_runtime.load_character(user_id)
		else:
			# Client also needs the player in Luau for position sync to work
			sbx_runtime.create_player(user_id, display_name)
			sbx_runtime.load_character(user_id)

	if not player_nodes.has(user_id):
		var is_local = (user_id == NetworkManager.local_player_id)
		_spawn_player_node(user_id, display_name, is_local)

	_update_player_list()

func _on_existing_player(user_id: int, display_name: String, position: Vector3) -> void:
	print("[GameManager] Existing player: ", display_name, " (ID: ", user_id, ") at ", position)

	# Create existing players in Luau for position sync
	if sbx_runtime and sbx_available:
		sbx_runtime.create_player(user_id, display_name)
		sbx_runtime.load_character(user_id)
		# Set the initial position in Luau
		sbx_runtime.set_player_position(user_id, position)

	if not player_nodes.has(user_id):
		_spawn_player_node(user_id, display_name, false, position)
	_update_player_list()

func _on_player_left(user_id: int) -> void:
	print("[GameManager] Player left: ", user_id)

	if NetworkManager.is_server and sbx_runtime and sbx_available:
		sbx_runtime.remove_player(user_id)

	if player_nodes.has(user_id):
		player_nodes[user_id].queue_free()
		player_nodes.erase(user_id)

	if player_materials.has(user_id):
		player_materials.erase(user_id)

	_update_player_list()

# Generic signal handlers - Luau controls rendering through these
func _on_player_color_changed(user_id: int, color: Color) -> void:
	if player_materials.has(user_id):
		player_materials[user_id].albedo_color = color

	# Sync color to clients via RPC
	if NetworkManager.is_server:
		_sync_player_color.rpc(user_id, color)

func _on_status_text_changed(text: String) -> void:
	status_label.text = text

	# Sync status to clients via RPC
	if NetworkManager.is_server:
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

	# Create material (default colors, Luau will override via set_player_color)
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

	# Initial position - use provided position or random if not given
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

	# Send input direction to Luau
	sbx_runtime.set_input_direction(NetworkManager.local_player_id, direction)

func _sync_positions_from_luau() -> void:
	# Get all player positions from Luau
	var positions: Dictionary = sbx_runtime.get_all_player_positions()

	# Update visual nodes to match Luau positions
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
	var user_id = NetworkManager.local_player_id

	if NetworkManager.is_server:
		_broadcast_position.rpc(user_id, current_pos)
	else:
		_send_position_to_server.rpc_id(1, user_id, current_pos)

@rpc("any_peer", "unreliable_ordered")
func _send_position_to_server(user_id: int, pos: Vector3) -> void:
	if not NetworkManager.is_server:
		return

	# Update Luau position
	if sbx_runtime and sbx_available:
		sbx_runtime.set_player_position(user_id, pos)

	# Broadcast to all clients
	_broadcast_position.rpc(user_id, pos)

@rpc("authority", "unreliable_ordered", "call_local")
func _broadcast_position(user_id: int, pos: Vector3) -> void:
	# Update Luau position (for clients)
	if sbx_runtime and sbx_available and user_id != NetworkManager.local_player_id:
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
		if user_id == NetworkManager.local_player_id:
			name_text += " (You)"
		player_list.add_item(name_text)

func _cleanup_game() -> void:
	for user_id in player_nodes:
		player_nodes[user_id].queue_free()
	player_nodes.clear()
	player_materials.clear()
	local_player_node = null
