extends Node3D

# GameManager - Main game controller for the Tag multiplayer demo
# Coordinates between Godot (rendering/input) and shadowblox (game logic)

var sbx_runtime = null  # Will be SbxRuntime if available
var sbx_available := false

@onready var ui_layer: CanvasLayer = $UILayer
@onready var status_label: Label = $UILayer/GameUI/StatusLabel
@onready var player_list: ItemList = $UILayer/GameUI/PlayerList
@onready var host_button: Button = $UILayer/MainMenu/HostButton
@onready var join_button: Button = $UILayer/MainMenu/JoinButton
@onready var address_input: LineEdit = $UILayer/MainMenu/AddressInput
@onready var name_input: LineEdit = $UILayer/MainMenu/NameInput
@onready var main_menu: Control = $UILayer/MainMenu
@onready var game_ui: Control = $UILayer/GameUI

var local_player_node: Node3D = null
var player_nodes := {}  # user_id -> Node3D
var game_started := false

func _ready() -> void:
	# Connect network manager signals
	NetworkManager.server_started.connect(_on_server_started)
	NetworkManager.client_connected.connect(_on_client_connected)
	NetworkManager.client_disconnected.connect(_on_client_disconnected)
	NetworkManager.player_joined.connect(_on_player_joined)
	NetworkManager.player_left.connect(_on_player_left)

	# Connect UI buttons
	host_button.pressed.connect(_on_host_pressed)
	join_button.pressed.connect(_on_join_pressed)

	# Initially show main menu
	main_menu.visible = true
	game_ui.visible = false

	# Check if SbxRuntime is available (extension loaded)
	if ClassDB.class_exists("SbxRuntime"):
		sbx_available = true
		# Create SbxRuntime dynamically to avoid scene-load crashes
		sbx_runtime = ClassDB.instantiate("SbxRuntime")
		if sbx_runtime:
			sbx_runtime.name = "SbxRuntime"
			add_child(sbx_runtime)
			print("[GameManager] ShadowBlox extension loaded - using Luau integration")
			# Wait a frame for SbxRuntime to initialize
			await get_tree().process_frame
			_setup_luau_game()
		else:
			print("[GameManager] Failed to instantiate SbxRuntime")
			sbx_available = false
	else:
		print("[GameManager] ShadowBlox extension not available")
		print("[GameManager] To build: cd shadowblox_godot && scons platform=windows target=template_debug")
		print("[GameManager] Game will run without Luau scripting")

func _setup_luau_game() -> void:
	# Load and execute the game initialization script
	var game_init_path = "res://luau/game_init.luau"
	var file = FileAccess.open(game_init_path, FileAccess.READ)
	if file:
		var code = file.get_as_text()
		file.close()
		var result = sbx_runtime.execute_script(code)
		if result != "OK":
			printerr("[GameManager] Failed to load game_init.luau: ", result)
		else:
			print("[GameManager] Loaded game_init.luau")
	else:
		print("[GameManager] game_init.luau not found, will load when available")

func _on_host_pressed() -> void:
	var display_name = name_input.text if name_input.text != "" else "Host"
	var error = NetworkManager.start_server()
	if error == OK:
		main_menu.visible = false
		game_ui.visible = true

		# Set runtime as server
		if sbx_runtime and sbx_available:
			sbx_runtime.set_is_server(true)
			# Create local player
			sbx_runtime.create_player(1, display_name)
			sbx_runtime.load_character(1)

		_spawn_player_node(1, display_name, true)
		status_label.text = "Hosting - Waiting for players..."

func _on_join_pressed() -> void:
	var address = address_input.text if address_input.text != "" else "127.0.0.1"
	var display_name = name_input.text if name_input.text != "" else "Player"

	var error = NetworkManager.join_server(address, 7777, display_name)
	if error == OK:
		main_menu.visible = false
		game_ui.visible = true

		# Set runtime as client
		if sbx_runtime and sbx_available:
			sbx_runtime.set_is_client(true)

		status_label.text = "Connecting..."

func _on_server_started() -> void:
	print("[GameManager] Server started")
	status_label.text = "Server running - Waiting for players..."

func _on_client_connected() -> void:
	print("[GameManager] Connected to server")
	status_label.text = "Connected! Waiting for game..."

	# Create local player on client
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

	if NetworkManager.is_server and sbx_runtime and sbx_available:
		# Server creates the player in shadowblox
		sbx_runtime.create_player(user_id, display_name)
		sbx_runtime.load_character(user_id)

	# Spawn visual representation
	if not player_nodes.has(user_id):
		var is_local = (user_id == NetworkManager.local_player_id)
		_spawn_player_node(user_id, display_name, is_local)

	_update_player_list()
	_update_status()

func _on_player_left(user_id: int) -> void:
	print("[GameManager] Player left: ", user_id)

	if NetworkManager.is_server and sbx_runtime and sbx_available:
		sbx_runtime.remove_player(user_id)

	# Remove visual representation
	if player_nodes.has(user_id):
		player_nodes[user_id].queue_free()
		player_nodes.erase(user_id)

	_update_player_list()
	_update_status()

func _spawn_player_node(user_id: int, display_name: String, is_local: bool) -> void:
	# Create a simple visual representation of the player
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
	material.albedo_color = Color.BLUE if not is_local else Color.GREEN
	body.material_override = material

	player_node.add_child(body)

	# Add name label
	var label_3d = Label3D.new()
	label_3d.text = display_name
	label_3d.position.y = 1.5
	label_3d.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	player_node.add_child(label_3d)

	# Set initial position (spread players out)
	var spawn_offset = Vector3(randf_range(-5, 5), 1, randf_range(-5, 5))
	player_node.position = spawn_offset

	# Add collision if local player
	if is_local:
		local_player_node = player_node
		var camera = Camera3D.new()
		camera.position = Vector3(0, 5, 10)
		camera.look_at(Vector3.ZERO)
		player_node.add_child(camera)
		camera.current = true

	add_child(player_node)
	player_nodes[user_id] = player_node

func _process(delta: float) -> void:
	if local_player_node:
		_handle_input(delta)

	# Sync player positions from shadowblox (simplified for demo)
	_sync_player_positions()

func _handle_input(delta: float) -> void:
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
		var speed = 5.0
		local_player_node.position += direction * speed * delta

		# Keep within bounds
		local_player_node.position.x = clamp(local_player_node.position.x, -20, 20)
		local_player_node.position.z = clamp(local_player_node.position.z, -20, 20)

func _sync_player_positions() -> void:
	# In a full implementation, this would sync with the shadowblox runtime
	# For now, player movement is handled directly in Godot
	pass

func _update_player_list() -> void:
	player_list.clear()
	for user_id in player_nodes:
		var name_text = "Player " + str(user_id)
		if user_id == NetworkManager.local_player_id:
			name_text += " (You)"
		player_list.add_item(name_text)

func _update_status() -> void:
	var player_count = player_nodes.size()
	if NetworkManager.is_server:
		status_label.text = "Server - " + str(player_count) + " players"
	else:
		status_label.text = "Client - " + str(player_count) + " players"

func _cleanup_game() -> void:
	for user_id in player_nodes:
		player_nodes[user_id].queue_free()
	player_nodes.clear()
	local_player_node = null
	game_started = false
