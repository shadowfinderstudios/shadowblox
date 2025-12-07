extends Node

const MASTER_SERVER_URL = "http://localhost:9050"
const RELAY_HOST = "localhost"
const RELAY_TCP_PORT = 9998
const RELAY_UDP_PORT = 9999

var _peer: NodeTunnelPeerP2P
var _is_hosting: bool = false
var _game_state: Dictionary = {}

func _ready():
	_peer = NodeTunnelPeerP2P.new()
	_peer.setup_p2p_components(self)
	NodeTunnelPeer.debug_enabled = true

	_peer.relay_connected.connect(_on_relay_connected)
	_peer.hosting.connect(_on_hosting)
	_peer.joined.connect(_on_joined)
	_peer.room_left.connect(_on_room_left)

	_peer.p2p_connection_established.connect(_on_p2p_connection_established)
	_peer.p2p_connection_failed.connect(_on_p2p_connection_failed)
	_peer.host_migration_started.connect(_on_migration_started)
	_peer.host_migration_completed.connect(_on_migration_completed)
	_peer.host_migration_failed.connect(_on_migration_failed)
	_peer.connection_quality_updated.connect(_on_quality_updated)

	if _peer._host_migration != null:
		_peer._host_migration.state_backup_requested.connect(_on_state_backup_requested)
		_peer._host_migration.state_restore_requested.connect(_on_state_restore_requested)

	print("Host migration commands:")
	print("  'host' - Start hosting")
	print("  'join <host_id>' - Join a host")
	print("  'leave' - Leave room")
	print("  'migrate' - Force host migration")
	print("  'state' - Print game state")

func connect_to_servers():
	print("\n[Example] Connecting to servers...")
	_peer.set_p2p_enabled(true, MASTER_SERVER_URL)
	_peer.set_fallback_relay(RELAY_HOST, RELAY_TCP_PORT, RELAY_UDP_PORT)
	_peer.set_connection_mode(NodeTunnelPeerP2P.ConnectionMode.P2P_PREFERRED)
	await _peer.connect_to_master_and_relay(MASTER_SERVER_URL, RELAY_HOST, RELAY_TCP_PORT)
	print("[Example] Connected! Online ID: " + _peer.online_id)

func start_hosting():
	print("\n[Example] Starting P2P host...")
	var metadata = {
		"Name": "Example P2P Game",
		"Mode": "Deathmatch",
		"Map": "TestMap",
		"MaxPlayers": 4,
		"CurrentPlayers": 1,
		"IsPassworded": false,
		"Version": "1.0"
	}
	_peer.enable_lobby_registration(self, MASTER_SERVER_URL, metadata)
	await _peer.host_p2p()
	_is_hosting = true
	_initialize_game_state()
	print("[Example] Hosting started! Other players can join with your OID: " + _peer.online_id)

func join_session(host_oid: String):
	print("\n[Example] Joining session: " + host_oid)
	await _peer.join_p2p(host_oid)
	_initialize_game_state()
	print("[Example] Joined session!")

func leave_session():
	print("\n[Example] Leaving session...")
	_peer.leave_room()
	_is_hosting = false
	_game_state.clear()
	print("[Example] Left session")

func force_migration():
	if not _is_hosting:
		print("[Example] Not hosting, cannot force migration")
		return
	print("\n[Example] Forcing host migration...")
	_peer.trigger_host_migration(HostMigration.MigrationReason.MANUAL_REQUEST)

func _initialize_game_state():
	_game_state = {
		"players": {},
		"scores": {},
		"game_time": 0.0,
		"round": 1
	}

	_game_state.players[_peer.online_id] = {
		"position": Vector3.ZERO,
		"rotation": 0.0,
		"health": 100,
		"team": 0
	}
	_game_state.scores[_peer.online_id] = 0

	print("[Example] Game state initialized")

## Update game state (called every frame in real game)
func _update_game_state(delta: float):
	if not _game_state.has("game_time"):
		return

	_game_state.game_time += delta

	# Simulate player movement
	for player_id in _game_state.players.keys():
		var player = _game_state.players[player_id]
		# Would update based on actual gameplay
		pass

## Backup game state for migration (host only)
func _on_state_backup_requested():
	print("[Example] Backing up game state for migration...")

	# Serialize current game state
	var backup = _serialize_game_state()

	# Store for transfer
	_peer._host_migration._game_state_backup = backup

	print("[Example] State backed up (%d bytes)" % JSON.stringify(backup).length())

## Restore game state after migration (new host)
func _on_state_restore_requested(state_data: Dictionary):
	print("[Example] Restoring game state after migration...")

	_deserialize_game_state(state_data)

	print("[Example] State restored!")

## Serialize game state for transfer
func _serialize_game_state() -> Dictionary:
	return {
		"version": 1,
		"timestamp": Time.get_unix_time_from_system(),
		"game_time": _game_state.get("game_time", 0.0),
		"round": _game_state.get("round", 1),
		"players": _game_state.get("players", {}).duplicate(true),
		"scores": _game_state.get("scores", {}).duplicate(true)
	}

## Deserialize game state after transfer
func _deserialize_game_state(state_data: Dictionary):
	if state_data.get("version", 0) != 1:
		push_error("[Example] Incompatible state version")
		return

	_game_state = {
		"game_time": state_data.get("game_time", 0.0),
		"round": state_data.get("round", 1),
		"players": state_data.get("players", {}),
		"scores": state_data.get("scores", {})
	}

## Print current game state
func print_game_state():
	print("\n=== Game State ===")
	print("Time: %.1f seconds" % _game_state.get("game_time", 0.0))
	print("Round: %d" % _game_state.get("round", 1))
	print("Players:")
	for player_id in _game_state.get("players", {}).keys():
		var player = _game_state.players[player_id]
		var score = _game_state.scores.get(player_id, 0)
		print("  %s - Health: %d, Score: %d" % [player_id, player.get("health", 0), score])
	print("=================\n")

func _on_relay_connected(online_id: String):
	print("[Example] Connected to relay: " + online_id)

func _on_hosting():
	print("[Example] Hosting started")

func _on_joined():
	print("[Example] Joined session")

func _on_room_left():
	print("[Example] Left room")

func _on_p2p_connection_established(peer_id: String):
	print("[Example] P2P connection established: " + peer_id)

func _on_p2p_connection_failed(peer_id: String):
	print("[Example] P2P connection failed (using fallback): " + peer_id)

func _on_migration_started(reason: String):
	print("\n[Example]  HOST MIGRATION STARTED")
	print("  Reason: " + reason)
	print("  Please wait...")

func _on_migration_completed(new_host_id: String):
	print("\n[Example] HOST MIGRATION COMPLETED")
	print("  New host: " + new_host_id)

	_is_hosting = (new_host_id == _peer.online_id)

	if _is_hosting:
		print("  You are now the host!")
	else:
		print("  Reconnected to new host")

func _on_migration_failed(reason: String):
	print("\n[Example] HOST MIGRATION FAILED")
	print("  Reason: " + reason)

func _on_quality_updated(metrics: Dictionary):
	var score = _calculate_quality_score(metrics)
	if score < 0.5:
		print("[Example] Connection quality degraded (Score: %.2f)" % score)

func _calculate_quality_score(metrics: Dictionary) -> float:
	var ping_ms = metrics.get("ping_ms", 0.0)
	var stability = metrics.get("stability_score", 1.0)
	var norm_ping = max(1.0 - (ping_ms / 300.0), 0.0)
	return 0.25 + (norm_ping * 0.3) + (stability * 0.2)

func _input(event):
	if event is InputEventKey and event.pressed:
		# This would normally be handled by UI
		pass

## Process text command (for demo)
func process_command(command: String):
	var parts = command.split(" ", false)
	if parts.is_empty():
		return

	match parts[0].to_lower():
		"connect":
			await connect_to_servers()

		"host":
			if _peer.connection_state == NodeTunnelPeer.ConnectionState.DISCONNECTED:
				await connect_to_servers()
			await start_hosting()

		"join":
			if parts.size() < 2:
				print("[Example] Usage: join <host_id>")
				return
			if _peer.connection_state == NodeTunnelPeer.ConnectionState.DISCONNECTED:
				await connect_to_servers()
			await join_session(parts[1])

		"leave":
			leave_session()

		"migrate":
			force_migration()

		"state":
			print_game_state()

		"help":
			print("\nCommands:")
			print("  connect - Connect to servers")
			print("  host - Start hosting")
			print("  join <host_id> - Join a session")
			print("  leave - Leave current session")
			print("  migrate - Force migration (host only)")
			print("  state - Print game state")
			print("  help - Show this help")

		_:
			print("[Example] Unknown command: " + parts[0])
			print("Type 'help' for available commands")

# ============================================================================
# AUTOMATED DEMO
# ============================================================================

## Run automated demo sequence
func run_demo():
	print("\n=== Running Automated Demo ===\n")

	# Step 1: Connect
	await connect_to_servers()
	await get_tree().create_timer(1.0).timeout

	# Step 2: Start hosting
	await start_hosting()
	await get_tree().create_timer(2.0).timeout

	# Step 3: Simulate some gameplay
	print("\n[Example] Simulating gameplay...")
	for i in range(5):
		_update_game_state(1.0)
		print("  Game time: %.1f seconds" % _game_state.game_time)
		await get_tree().create_timer(1.0).timeout

	# Step 4: Force migration
	print("\n[Example] Testing host migration...")
	force_migration()
	await get_tree().create_timer(2.0).timeout

	# Step 5: Continue after migration
	print("\n[Example] Resuming gameplay after migration...")
	for i in range(3):
		_update_game_state(1.0)
		print("  Game time: %.1f seconds" % _game_state.game_time)
		await get_tree().create_timer(1.0).timeout

	# Step 6: Print final state
	print_game_state()

	print("\n=== Demo Complete ===\n")
