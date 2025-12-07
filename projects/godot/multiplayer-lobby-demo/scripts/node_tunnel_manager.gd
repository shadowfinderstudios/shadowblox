extends Node

# NodeTunnelManager - Thin wrapper around NodeTunnel for shadowblox
# This is a minimal network transport layer - all game logic is in Luau
# Replaces the old ENet-based NetworkManager

## Network topology configuration
enum NetworkTopology {
	RELAY_ONLY,    # Traditional relay server
	P2P_PREFERRED, # P2P with relay fallback (experimental)
	P2P_ONLY       # P2P only (experimental)
}

const NETWORK_TOPOLOGY: NetworkTopology = NetworkTopology.P2P_PREFERRED
const DEFAULT_MASTER_SERVER: String = "http://localhost:9050"
const MAX_PLAYERS: int = 16

# NodeTunnel peer
var peer: NodeTunnelPeer = null
var relay_ready: bool = false

# Connection state
var is_server: bool = false
var is_client: bool = false
var local_player_id: int = 0
var local_online_id: String = ""
var host_online_id: String = ""
var local_display_name: String = ""

# Player tracking (online_id -> {user_id, display_name})
var player_info: Dictionary = {}

# Host migration state
var migration_in_progress: bool = false
var players_before_migration: Dictionary = {}

# World state for new player sync
var world_state_data: Dictionary = {}

# Signals for game manager to connect to
signal relay_connected(online_id: String)
signal hosting_started
signal joined_lobby(host_id: String)
signal registration_complete(user_id: int)  # Fired when server assigns our user_id
signal left_room
signal player_joined(user_id: int, display_name: String, online_id: String)
signal player_left(user_id: int, online_id: String)
signal existing_player(user_id: int, display_name: String, position: Vector3)
signal world_state_received(state: Dictionary)
signal host_migration_started(reason: String)
signal host_migration_completed(new_host_id: String)
signal host_migration_failed(reason: String)


func _ready() -> void:
	# Initialize peer based on configured topology
	_initialize_peer()

	multiplayer.multiplayer_peer = peer
	peer.set_encryption_enabled(true)

	# Connect to relay server
	var relay: String = OS.get_environment("NODETUNNEL_RELAY")
	if relay:
		_connect_to_network(relay)
	else:
		push_error("[NodeTunnelManager] Set NODETUNNEL_RELAY environment variable")
		return


func _initialize_peer() -> void:
	match NETWORK_TOPOLOGY:
		NetworkTopology.RELAY_ONLY:
			print("[NodeTunnelManager] Initializing in RELAY_ONLY mode")
			peer = NodeTunnelPeer.new()

		NetworkTopology.P2P_PREFERRED, NetworkTopology.P2P_ONLY:
			print("[NodeTunnelManager] Initializing in P2P mode")
			peer = NodeTunnelPeerP2P.new()
			(peer as NodeTunnelPeerP2P).setup_p2p_components(self)

			var master_server = OS.get_environment("NODETUNNEL_MASTER")
			if master_server.is_empty():
				master_server = DEFAULT_MASTER_SERVER

			(peer as NodeTunnelPeerP2P).set_p2p_enabled(true, master_server)
			(peer as NodeTunnelPeerP2P).enable_host_migration = true

			if NETWORK_TOPOLOGY == NetworkTopology.P2P_PREFERRED:
				(peer as NodeTunnelPeerP2P).set_connection_mode(NodeTunnelPeerP2P.ConnectionMode.P2P_PREFERRED)
			else:
				(peer as NodeTunnelPeerP2P).set_connection_mode(NodeTunnelPeerP2P.ConnectionMode.P2P_ONLY)

			# Connect P2P-specific signals
			(peer as NodeTunnelPeerP2P).p2p_connection_established.connect(_on_p2p_connection_established)
			(peer as NodeTunnelPeerP2P).p2p_connection_failed.connect(_on_p2p_connection_failed)
			(peer as NodeTunnelPeerP2P).host_migration_started.connect(_on_host_migration_started)
			(peer as NodeTunnelPeerP2P).host_migration_completed.connect(_on_host_migration_completed)
			(peer as NodeTunnelPeerP2P).host_migration_failed.connect(_on_host_migration_failed)


func _connect_to_network(relay_address: String) -> void:
	match NETWORK_TOPOLOGY:
		NetworkTopology.RELAY_ONLY:
			peer.connect_to_relay(relay_address, 9998)
			await peer.relay_connected
			_on_relay_connected()

		NetworkTopology.P2P_PREFERRED, NetworkTopology.P2P_ONLY:
			var master_server = OS.get_environment("NODETUNNEL_MASTER")
			if master_server.is_empty():
				master_server = DEFAULT_MASTER_SERVER

			if not master_server.begins_with("http://") and not master_server.begins_with("https://"):
				master_server = "http://" + master_server

			print("[NodeTunnelManager] Connecting to master: ", master_server)
			print("[NodeTunnelManager] Connecting to relay: ", relay_address)

			(peer as NodeTunnelPeerP2P).set_fallback_relay(relay_address, 9998, 9999)
			await (peer as NodeTunnelPeerP2P).connect_to_master_and_relay(master_server, relay_address, 9998)
			_on_relay_connected()


func _on_relay_connected() -> void:
	local_online_id = peer.online_id
	relay_ready = true

	# Connect peer signals
	peer.peer_connected.connect(_on_peer_connected)
	peer.peer_disconnected.connect(_on_peer_disconnected)
	peer.room_left.connect(_on_room_left)

	print("[NodeTunnelManager] Connected. Online ID: ", local_online_id)
	relay_connected.emit(local_online_id)


# ============================================================================
# PUBLIC API - Called by GameManager
# ============================================================================

func is_relay_ready() -> bool:
	return relay_ready


func get_online_id() -> String:
	return local_online_id


func get_topology_name() -> String:
	match NETWORK_TOPOLOGY:
		NetworkTopology.RELAY_ONLY:
			return "RELAY"
		NetworkTopology.P2P_PREFERRED:
			return "P2P+RELAY"
		NetworkTopology.P2P_ONLY:
			return "P2P ONLY"
	return "UNKNOWN"


func host_game(lobby_name: String, display_name: String) -> void:
	if not relay_ready:
		push_error("[NodeTunnelManager] Not connected to relay")
		return

	local_display_name = display_name

	var metadata = LobbyMetadata.create(
		lobby_name,
		"Tag",
		"Arena",
		MAX_PLAYERS,
		false,
		{
			"Version": "1.0.0",
			"Region": "Auto"
		}
	)

	match NETWORK_TOPOLOGY:
		NetworkTopology.RELAY_ONLY:
			var registry: String = OS.get_environment("NODETUNNEL_REGISTRY")
			if registry and not registry.is_empty():
				if not registry.begins_with("http://") and not registry.begins_with("https://"):
					registry = "http://" + registry
				if not registry.contains(":8099"):
					registry = registry + ":8099"
				peer.enable_lobby_registration(self, registry, metadata)
			peer.host()
			await peer.hosting

		NetworkTopology.P2P_PREFERRED, NetworkTopology.P2P_ONLY:
			peer.lobby_metadata = metadata
			await (peer as NodeTunnelPeerP2P).host_p2p()

	# We're now hosting
	is_server = true
	is_client = false
	local_player_id = 1
	host_online_id = local_online_id

	# Register ourselves
	player_info[local_online_id] = {
		"user_id": 1,
		"display_name": display_name
	}

	print("[NodeTunnelManager] Now hosting. Lobby: ", lobby_name)
	hosting_started.emit()


func join_game(host_id: String, display_name: String) -> void:
	if not relay_ready:
		push_error("[NodeTunnelManager] Not connected to relay")
		return

	local_display_name = display_name
	host_online_id = host_id

	match NETWORK_TOPOLOGY:
		NetworkTopology.RELAY_ONLY:
			peer.join(host_id)
			await peer.joined

		NetworkTopology.P2P_PREFERRED, NetworkTopology.P2P_ONLY:
			await (peer as NodeTunnelPeerP2P).join_p2p(host_id)

	is_server = false
	is_client = true

	print("[NodeTunnelManager] Joined lobby: ", host_id)
	joined_lobby.emit(host_id)

	# Register with the server - this will assign our user_id
	# The server will respond with _receive_user_id which sets local_player_id
	# and emits registration_complete signal
	register_player.rpc_id(1, display_name)

	# Request world state from server (for existing players)
	fetch_world_state.rpc_id(1)


func leave_game() -> void:
	if peer:
		peer.leave_room()


func disconnect_from_relay() -> void:
	if peer:
		peer.close()
		relay_ready = false


# ============================================================================
# PEER EVENTS
# ============================================================================

func _on_peer_connected(peer_id: int) -> void:
	if not is_server:
		return

	print("[NodeTunnelManager] Peer connected: ", peer_id)
	# The peer will register themselves via RPC


func _on_peer_disconnected(peer_id: int) -> void:
	print("[NodeTunnelManager] Peer disconnected: ", peer_id)

	# Skip removal during migration
	if migration_in_progress:
		print("[NodeTunnelManager] Skipping removal during migration")
		return

	# Find and remove the player
	var online_id_to_remove = ""
	var user_id_to_remove = 0

	for oid in player_info:
		if player_info[oid].user_id == peer_id:
			online_id_to_remove = oid
			user_id_to_remove = peer_id
			break

	if online_id_to_remove != "":
		player_info.erase(online_id_to_remove)
		player_left.emit(user_id_to_remove, online_id_to_remove)

		# Notify other clients
		if is_server:
			_broadcast_player_left.rpc(user_id_to_remove, online_id_to_remove)


func _on_room_left() -> void:
	print("[NodeTunnelManager] Left room")

	is_server = false
	is_client = false
	local_player_id = 0
	player_info.clear()
	world_state_data.clear()

	left_room.emit()


# ============================================================================
# HOST MIGRATION
# ============================================================================

func _on_p2p_connection_established(peer_id: String) -> void:
	print("[NodeTunnelManager] P2P established: ", peer_id)


func _on_p2p_connection_failed(peer_id: String) -> void:
	if NETWORK_TOPOLOGY == NetworkTopology.P2P_PREFERRED:
		print("[NodeTunnelManager] P2P failed, using relay: ", peer_id)


func _on_host_migration_started(reason: String) -> void:
	print("[NodeTunnelManager] Host migration started: ", reason)
	migration_in_progress = true
	players_before_migration = player_info.duplicate(true)
	host_migration_started.emit(reason)


func _on_host_migration_completed(new_host_id: String) -> void:
	print("[NodeTunnelManager] Host migration completed. New host: ", new_host_id)
	migration_in_progress = false

	# Wait for multiplayer state to update
	await get_tree().process_frame
	await get_tree().process_frame

	host_online_id = new_host_id

	# Check if we became the host
	if multiplayer.is_server():
		print("[NodeTunnelManager] We are the new host!")
		is_server = true
		is_client = false

		# Update our user_id to 1
		var old_user_id = local_player_id
		local_player_id = 1

		if player_info.has(local_online_id):
			player_info[local_online_id].user_id = 1

	host_migration_completed.emit(new_host_id)
	players_before_migration.clear()


func _on_host_migration_failed(reason: String) -> void:
	print("[NodeTunnelManager] Host migration failed: ", reason)
	migration_in_progress = false
	players_before_migration.clear()
	host_migration_failed.emit(reason)


# ============================================================================
# RPC METHODS - Network communication
# ============================================================================

@rpc("any_peer", "reliable")
func register_player(display_name: String) -> void:
	if not is_server:
		return

	var sender_id = multiplayer.get_remote_sender_id()
	var sender_online_id = _get_online_id_from_peer_id(sender_id)

	player_info[sender_online_id] = {
		"user_id": sender_id,
		"display_name": display_name
	}

	print("[NodeTunnelManager] Player registered: ", display_name, " (", sender_id, ")")

	# Send user ID back
	_receive_user_id.rpc_id(sender_id, sender_id, sender_online_id)

	# Broadcast to all clients
	_broadcast_player_joined.rpc(sender_id, display_name, sender_online_id)

	player_joined.emit(sender_id, display_name, sender_online_id)


@rpc("authority", "reliable")
func _receive_user_id(user_id: int, online_id: String) -> void:
	local_player_id = user_id
	local_online_id = online_id
	print("[NodeTunnelManager] Received user ID: ", user_id)

	# Now that we have our user_id, GameManager can create our local player
	registration_complete.emit(user_id)


@rpc("authority", "reliable")
func _broadcast_player_joined(user_id: int, display_name: String, online_id: String) -> void:
	if user_id == local_player_id:
		return
	player_joined.emit(user_id, display_name, online_id)


@rpc("authority", "reliable")
func _broadcast_player_left(user_id: int, online_id: String) -> void:
	player_left.emit(user_id, online_id)


# ============================================================================
# WORLD STATE SYNC
# ============================================================================

@rpc("any_peer", "reliable")
func fetch_world_state() -> void:
	if not is_server:
		return

	var sender_id = multiplayer.get_remote_sender_id()

	# Build world state including all player info
	var state: Dictionary = {
		"players": {},
		"host_online_id": host_online_id
	}

	for oid in player_info:
		var info = player_info[oid]
		state.players[oid] = {
			"user_id": info.user_id,
			"display_name": info.display_name
		}

	# Game manager will add positions and game state

	receive_world_state.rpc_id(sender_id, state)


@rpc("authority", "reliable")
func receive_world_state(state: Dictionary) -> void:
	if is_server:
		return

	print("[NodeTunnelManager] Received world state")
	world_state_data = state

	# Process existing players
	if state.has("players"):
		for oid in state.players:
			var pdata = state.players[oid]
			if pdata.user_id != local_player_id:
				player_info[oid] = pdata
				existing_player.emit(pdata.user_id, pdata.display_name, Vector3.ZERO)

	world_state_received.emit(state)


# ============================================================================
# POSITION SYNC (Called by GameManager)
# ============================================================================

@rpc("any_peer", "unreliable_ordered")
func send_position(user_id: int, pos: Vector3) -> void:
	if not is_server:
		return
	# Server received position from client, broadcast to all
	broadcast_position.rpc(user_id, pos)


@rpc("authority", "unreliable_ordered", "call_local")
func broadcast_position(user_id: int, pos: Vector3) -> void:
	# This will be received by GameManager via signal or direct call
	pass  # GameManager handles position updates via its own sync


# ============================================================================
# HELPERS
# ============================================================================

func _get_online_id_from_peer_id(peer_id: int) -> String:
	# NodeTunnel provides a mapping
	if peer and peer.has_method("get_online_id_for_peer"):
		return peer.get_online_id_for_peer(peer_id)

	# Fallback: use peer_id as string
	return str(peer_id)


func get_all_player_ids() -> Array[int]:
	var ids: Array[int] = []
	for oid in player_info:
		ids.append(player_info[oid].user_id)
	return ids


func get_player_display_name(user_id: int) -> String:
	for oid in player_info:
		if player_info[oid].user_id == user_id:
			return player_info[oid].display_name
	return "Unknown"
