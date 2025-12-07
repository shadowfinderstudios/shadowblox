extends Node

# NetworkManager - Handles ENet multiplayer networking for shadowblox
# This bridges Godot's multiplayer API with the shadowblox runtime

const DEFAULT_PORT = 7777
const MAX_CLIENTS = 16

var peer: ENetMultiplayerPeer = null
var is_server := false
var is_client := false
var local_player_id := 0

# Player info storage (server-side)
var player_info := {}  # peer_id -> {user_id, display_name}
var host_display_name := "Host"  # Store host name for syncing to clients

signal server_started
signal server_stopped
signal client_connected
signal client_disconnected
signal player_joined(user_id: int, display_name: String)
signal player_left(user_id: int)
signal existing_player(user_id: int, display_name: String, position: Vector3)  # For syncing existing players to new clients

func _ready() -> void:
	multiplayer.peer_connected.connect(_on_peer_connected)
	multiplayer.peer_disconnected.connect(_on_peer_disconnected)
	multiplayer.connected_to_server.connect(_on_connected_to_server)
	multiplayer.connection_failed.connect(_on_connection_failed)
	multiplayer.server_disconnected.connect(_on_server_disconnected)

func start_server(port: int = DEFAULT_PORT, display_name: String = "Host") -> Error:
	peer = ENetMultiplayerPeer.new()
	var error = peer.create_server(port, MAX_CLIENTS)
	if error != OK:
		printerr("[NetworkManager] Failed to create server: ", error)
		return error

	multiplayer.multiplayer_peer = peer
	is_server = true
	is_client = false
	local_player_id = 1
	host_display_name = display_name

	print("[NetworkManager] Server started on port ", port)
	server_started.emit()
	return OK

func stop_server() -> void:
	if peer:
		peer.close()
		peer = null
	multiplayer.multiplayer_peer = null
	is_server = false
	player_info.clear()
	print("[NetworkManager] Server stopped")
	server_stopped.emit()

func join_server(address: String, port: int = DEFAULT_PORT, display_name: String = "Player") -> Error:
	peer = ENetMultiplayerPeer.new()
	var error = peer.create_client(address, port)
	if error != OK:
		printerr("[NetworkManager] Failed to connect: ", error)
		return error

	multiplayer.multiplayer_peer = peer
	is_client = true
	is_server = false

	# Store our display name for later registration
	set_meta("pending_display_name", display_name)

	print("[NetworkManager] Connecting to ", address, ":", port)
	return OK

func disconnect_from_server() -> void:
	if peer:
		peer.close()
		peer = null
	multiplayer.multiplayer_peer = null
	is_client = false
	print("[NetworkManager] Disconnected from server")
	client_disconnected.emit()

func _on_peer_connected(id: int) -> void:
	print("[NetworkManager] Peer connected: ", id)
	if is_server:
		# Get positions from game manager
		var game_manager = _get_game_manager()

		# Build full player list including host with positions
		var all_players = {
			1: {"user_id": 1, "display_name": host_display_name, "position": _get_player_position(1, game_manager)}
		}
		for peer_id in player_info:
			var user_id = player_info[peer_id].user_id
			all_players[peer_id] = {
				"user_id": user_id,
				"display_name": player_info[peer_id].display_name,
				"position": _get_player_position(user_id, game_manager)
			}
		# Send server info to the new client
		rpc_id(id, "_receive_server_info", all_players)

func _on_peer_disconnected(id: int) -> void:
	print("[NetworkManager] Peer disconnected: ", id)
	if is_server and player_info.has(id):
		var info = player_info[id]
		player_info.erase(id)
		player_left.emit(info.user_id)

		# Notify other clients
		rpc("_on_player_left", info.user_id)

func _on_connected_to_server() -> void:
	print("[NetworkManager] Connected to server")
	local_player_id = multiplayer.get_unique_id()
	is_client = true

	# Register with server
	var display_name = get_meta("pending_display_name", "Player")
	rpc_id(1, "_register_player", display_name)

	client_connected.emit()

func _on_connection_failed() -> void:
	printerr("[NetworkManager] Connection failed")
	peer = null
	multiplayer.multiplayer_peer = null
	is_client = false
	client_disconnected.emit()

func _on_server_disconnected() -> void:
	print("[NetworkManager] Server disconnected")
	is_client = false
	peer = null
	multiplayer.multiplayer_peer = null
	client_disconnected.emit()

@rpc("any_peer", "reliable")
func _register_player(display_name: String) -> void:
	if not is_server:
		return

	var sender_id = multiplayer.get_remote_sender_id()
	var user_id = sender_id  # Use peer ID as user ID for simplicity

	player_info[sender_id] = {
		"user_id": user_id,
		"display_name": display_name
	}

	print("[NetworkManager] Player registered: ", display_name, " (ID: ", user_id, ")")

	# Notify the player of their user_id
	rpc_id(sender_id, "_receive_user_id", user_id)

	# Notify all clients of the new player
	rpc("_on_player_joined", user_id, display_name)

	player_joined.emit(user_id, display_name)

@rpc("authority", "reliable")
func _receive_user_id(user_id: int) -> void:
	local_player_id = user_id
	print("[NetworkManager] Received user ID: ", user_id)

@rpc("authority", "reliable")
func _receive_server_info(info: Dictionary) -> void:
	print("[NetworkManager] Received server info with ", info.size(), " players")
	# Spawn all existing players
	for peer_id in info:
		var player_data = info[peer_id]
		var user_id = player_data.user_id
		var display_name = player_data.display_name
		var position = player_data.get("position", Vector3.ZERO)
		# Don't spawn ourselves - we handle that in _on_client_connected
		if user_id != local_player_id:
			print("[NetworkManager] Existing player: ", display_name, " (ID: ", user_id, ") at ", position)
			existing_player.emit(user_id, display_name, position)

@rpc("authority", "reliable")
func _on_player_joined(user_id: int, display_name: String) -> void:
	print("[NetworkManager] Player joined: ", display_name, " (ID: ", user_id, ")")
	player_joined.emit(user_id, display_name)

@rpc("authority", "reliable")
func _on_player_left(user_id: int) -> void:
	print("[NetworkManager] Player left: ", user_id)
	player_left.emit(user_id)

# Network event forwarding for RemoteEvents
func send_event_to_server(event_name: String, data: PackedByteArray) -> void:
	if is_client:
		rpc_id(1, "_receive_event_from_client", event_name, data)

func send_event_to_client(event_name: String, target_id: int, data: PackedByteArray) -> void:
	if is_server:
		if target_id < 0:
			# Send to all clients
			rpc("_receive_event_from_server", event_name, data)
		else:
			# Find peer ID for user ID
			for peer_id in player_info:
				if player_info[peer_id].user_id == target_id:
					rpc_id(peer_id, "_receive_event_from_server", event_name, data)
					break

@rpc("any_peer", "reliable")
func _receive_event_from_client(event_name: String, data: PackedByteArray) -> void:
	if not is_server:
		return
	var sender_id = multiplayer.get_remote_sender_id()
	var user_id = player_info.get(sender_id, {}).get("user_id", 0)

	# Forward to SbxRuntime
	var sbx_runtime = _get_sbx_runtime()
	if sbx_runtime:
		sbx_runtime.on_network_event(event_name, user_id, data)

@rpc("authority", "reliable")
func _receive_event_from_server(event_name: String, data: PackedByteArray) -> void:
	# Forward to SbxRuntime
	var sbx_runtime = _get_sbx_runtime()
	if sbx_runtime:
		sbx_runtime.on_network_event(event_name, 0, data)

func _get_sbx_runtime():
	# Find SbxRuntime in the scene
	var root = get_tree().root
	for child in root.get_children():
		var runtime = _find_sbx_runtime(child)
		if runtime:
			return runtime
	return null

func _find_sbx_runtime(node: Node):
	if node.get_class() == "SbxRuntime":
		return node
	for child in node.get_children():
		var result = _find_sbx_runtime(child)
		if result:
			return result
	return null

func get_all_player_ids() -> Array[int]:
	var ids: Array[int] = []
	if is_server:
		for peer_id in player_info:
			ids.append(player_info[peer_id].user_id)
	return ids

func get_player_display_name(user_id: int) -> String:
	if is_server:
		for peer_id in player_info:
			if player_info[peer_id].user_id == user_id:
				return player_info[peer_id].display_name
	return "Unknown"

func _get_game_manager() -> Node:
	var root = get_tree().root
	for child in root.get_children():
		if child.has_node("SbxRuntime"):
			return child
	return null

func _get_player_position(user_id: int, game_manager: Node) -> Vector3:
	if game_manager == null:
		return Vector3.ZERO
	if game_manager.has_method("get_player_position"):
		return game_manager.get_player_position(user_id)
	# Fallback: check player_nodes directly
	if "player_nodes" in game_manager and game_manager.player_nodes.has(user_id):
		return game_manager.player_nodes[user_id].position
	return Vector3.ZERO
