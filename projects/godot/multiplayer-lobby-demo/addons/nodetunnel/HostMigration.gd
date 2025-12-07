extends Node
class_name HostMigration

## Host migration coordinator for seamless host transitions
##
## This class handles the migration process when the current host leaves
## or becomes unavailable, including state transfer and reconnection.

signal migration_started(reason: String)
signal migration_phase_changed(phase: int)
signal migration_completed(new_host_id: String)
signal migration_failed(reason: String)
signal state_backup_requested()
signal state_restore_requested(state_data: Dictionary)

enum MigrationPhase {
	IDLE,
	DETECTING,
	SELECTING_HOST,
	TRANSFERRING_STATE,
	RECONNECTING,
	COMPLETED,
	FAILED
}

enum MigrationReason {
	HOST_LEFT,
	HOST_TIMEOUT,
	QUALITY_DEGRADED,
	MANUAL_REQUEST
}

const MIGRATION_TIMEOUT = 10.0  # seconds
const HEARTBEAT_INTERVAL = 1.0
const HEARTBEAT_TIMEOUT = 3.0

var _master_server_url: String = ""
var _session_id: String = ""
var _current_host_id: String = ""
var _my_peer_id: String = ""
var _am_i_host: bool = false

var _current_phase: int = MigrationPhase.IDLE
var _migration_start_time: float = 0.0
var _last_host_heartbeat: float = 0.0

var _peer_list: Array = []
var _game_state_backup: Dictionary = {}

## Initialize host migration
func initialize(master_url: String, session_id: String, peer_id: String) -> void:
	_master_server_url = master_url
	_session_id = session_id
	_my_peer_id = peer_id

	print("[Migration] Initialized for session %s" % _session_id)

	# Start heartbeat monitoring
	_start_heartbeat_monitoring()

## Set current host
func set_current_host(host_id: String, am_i_host: bool) -> void:
	_current_host_id = host_id
	_am_i_host = am_i_host
	_last_host_heartbeat = Time.get_unix_time_from_system()

	print("[Migration] Current host: %s (Am I host: %s)" % [host_id, am_i_host])

## Update peer list
func update_peer_list(peers: Array) -> void:
	_peer_list = peers

## Receive heartbeat from host
func receive_host_heartbeat() -> void:
	_last_host_heartbeat = Time.get_unix_time_from_system()

## Detect host timeout
func _check_host_timeout() -> void:
	# DISABLED: Heartbeat system not implemented yet
	# We rely on peer_disconnected signal from relay instead
	# This prevents false positives that trigger unwanted migrations
	return

	# Original code (disabled):
	# if _am_i_host or _current_phase != MigrationPhase.IDLE:
	# 	return
	#
	# var now = Time.get_unix_time_from_system()
	# if now - _last_host_heartbeat > HEARTBEAT_TIMEOUT:
	# 	print("[Migration] Host timeout detected!")
	# 	initiate_migration(MigrationReason.HOST_TIMEOUT)

## Start heartbeat monitoring
func _start_heartbeat_monitoring() -> void:
	var timer = Timer.new()
	add_child(timer)
	timer.wait_time = HEARTBEAT_INTERVAL
	timer.timeout.connect(_check_host_timeout)
	timer.start()

## Initiate migration process
func initiate_migration(reason: int) -> void:
	if _current_phase != MigrationPhase.IDLE:
		push_warning("[Migration] Migration already in progress")
		return

	# Validate that migration is properly initialized
	if _master_server_url.is_empty():
		push_error("[Migration] Cannot initiate migration - master server URL not set")
		return

	if _session_id.is_empty():
		push_error("[Migration] Cannot initiate migration - session ID not set")
		return

	if _my_peer_id.is_empty():
		push_error("[Migration] Cannot initiate migration - peer ID not set")
		return

	if _current_host_id.is_empty():
		push_error("[Migration] Cannot initiate migration - current host ID not set")
		return

	print("[Migration] Initiating migration - Reason: %s" % _reason_to_string(reason))

	_current_phase = MigrationPhase.DETECTING
	_migration_start_time = Time.get_unix_time_from_system()

	migration_started.emit(_reason_to_string(reason))
	migration_phase_changed.emit(_current_phase)

	# Request state backup if we're the host
	if _am_i_host:
		state_backup_requested.emit()

	# Start migration sequence
	await _perform_migration(reason)

## Perform the migration sequence
func _perform_migration(reason: int) -> void:
	# Phase 1: Select new host
	_current_phase = MigrationPhase.SELECTING_HOST
	migration_phase_changed.emit(_current_phase)

	var new_host_info = await _select_new_host(reason)

	if new_host_info == null or new_host_info.is_empty() or not new_host_info.has("newHostId"):
		_fail_migration("Failed to select new host - no response or invalid response from master server")
		return

	# Validate required fields
	if not new_host_info.has("success") or not new_host_info.get("success", false):
		var error_msg = new_host_info.get("message", "Unknown error")
		_fail_migration("Master server could not select new host: " + error_msg)
		return

	print("[Migration] New host selected: %s" % new_host_info.newHostId)

	# Phase 2: Transfer state
	_current_phase = MigrationPhase.TRANSFERRING_STATE
	migration_phase_changed.emit(_current_phase)

	var state_transferred = await _transfer_state(new_host_info)

	if not state_transferred:
		push_warning("[Migration] State transfer failed, continuing anyway")

	# Phase 3: Reconnect to new host
	_current_phase = MigrationPhase.RECONNECTING
	migration_phase_changed.emit(_current_phase)

	var reconnected = await _reconnect_to_new_host(new_host_info)

	if not reconnected:
		_fail_migration("Failed to reconnect to new host")
		return

	# Migration completed
	_current_phase = MigrationPhase.COMPLETED
	migration_phase_changed.emit(_current_phase)

	_current_host_id = new_host_info.newHostId
	_am_i_host = (_my_peer_id == new_host_info.newHostId)

	print("[Migration] Migration completed in %.2fs" %
		(Time.get_unix_time_from_system() - _migration_start_time))

	migration_completed.emit(new_host_info.newHostId)

	# Reset to idle after a delay
	await get_tree().create_timer(1.0).timeout
	_current_phase = MigrationPhase.IDLE

## Select new host via master server
func _select_new_host(reason: int) -> Dictionary:
	var http = HTTPRequest.new()
	add_child(http)

	# Gather peer IDs
	var peer_ids = []
	for peer in _peer_list:
		if peer.peer_id != _current_host_id:  # Exclude old host
			peer_ids.append(peer.peer_id)

	var body = JSON.stringify({
		"currentHostId": _current_host_id,
		"sessionId": _session_id,
		"peerIds": peer_ids,
		"reason": reason
	})

	var url = _master_server_url + "/api/migration/initiate"
	var headers = ["Content-Type: application/json"]

	print("[Migration] Requesting new host from: ", url)

	http.request(url, headers, HTTPClient.METHOD_POST, body)
	var response = await http.request_completed

	var result = null
	if response[1] == 200:
		var json = JSON.new()
		var parse_result = json.parse(response[3].get_string_from_utf8())
		if parse_result == OK:
			var data = json.data
			print("[Migration] Master server response: ", JSON.stringify(data))
			if data.get("success", false):
				result = data
			else:
				push_error("[Migration] Master server returned success=false: " + data.get("message", "Unknown error"))
		else:
			push_error("[Migration] Failed to parse JSON response from master server")
	else:
		push_error("[Migration] Master server returned HTTP %d" % response[1])

	http.queue_free()
	return result if result != null else {}  # Return empty dict on failure

## Transfer game state
func _transfer_state(new_host_info: Dictionary) -> bool:
	# If we're the old host, we send our state
	if _am_i_host:
		var state = _serialize_game_state()
		return await _send_state_to_new_host(new_host_info.newHostId, state)

	# If we're a peer, we might send our local state as backup
	else:
		var local_state = _serialize_local_state()
		return await _send_backup_state(new_host_info.newHostId, local_state)

## Send state to new host
func _send_state_to_new_host(new_host_id: String, state: Dictionary) -> bool:
	print("[Migration] Transferring state to new host (%d bytes)" %
		JSON.stringify(state).length())

	# In a real implementation, this would send via direct connection or relay
	# For now, we'll just store it locally
	_game_state_backup = state

	# Wait a bit to simulate network transfer
	await get_tree().create_timer(0.3).timeout

	return true

## Send backup state
func _send_backup_state(new_host_id: String, state: Dictionary) -> bool:
	print("[Migration] Sending backup state to new host")

	# Similar to above
	await get_tree().create_timer(0.1).timeout

	return true

## Reconnect to new host
func _reconnect_to_new_host(new_host_info: Dictionary) -> bool:
	print("[Migration] Reconnecting to new host at %s:%d" %
		[new_host_info.newHostIp, new_host_info.newHostPort])

	# If we're the new host, start hosting
	if _my_peer_id == new_host_info.newHostId:
		return await _become_new_host(new_host_info)

	# Otherwise, connect to new host
	else:
		return await _connect_to_new_host(new_host_info)

## Become the new host
func _become_new_host(host_info: Dictionary) -> bool:
	print("[Migration] Becoming new host")

	# Restore state
	if not _game_state_backup.is_empty():
		state_restore_requested.emit(_game_state_backup)

	# Wait for hosting to initialize
	await get_tree().create_timer(0.5).timeout

	return true

## Connect to new host
func _connect_to_new_host(host_info: Dictionary) -> bool:
	print("[Migration] Connecting to new host")

	# This would trigger the actual connection logic in the main plugin
	# For now, simulate connection
	await get_tree().create_timer(0.3).timeout

	return true

## Serialize full game state (for host)
func _serialize_game_state() -> Dictionary:
	# This should be implemented by the game to include:
	# - Player positions, rotations, velocities
	# - Player stats (health, score, etc.)
	# - World objects and their states
	# - Game mode state (time, scores, flags, etc.)

	return {
		"version": 1,
		"timestamp": Time.get_unix_time_from_system(),
		"session_id": _session_id,
		"players": _serialize_players(),
		"world": _serialize_world_state(),
		"game_mode": _serialize_game_mode_state()
	}

## Serialize local state (for peers as backup)
func _serialize_local_state() -> Dictionary:
	return {
		"peer_id": _my_peer_id,
		"timestamp": Time.get_unix_time_from_system(),
		"local_player": _serialize_local_player()
	}

## Serialize players (placeholder)
func _serialize_players() -> Array:
	# This would be implemented by the game
	return []

## Serialize world state (placeholder)
func _serialize_world_state() -> Dictionary:
	# This would be implemented by the game
	return {}

## Serialize game mode state (placeholder)
func _serialize_game_mode_state() -> Dictionary:
	# This would be implemented by the game
	return {}

## Serialize local player (placeholder)
func _serialize_local_player() -> Dictionary:
	# This would be implemented by the game
	return {}

## Fail migration
func _fail_migration(reason: String) -> void:
	print("[Migration] Migration failed: %s" % reason)

	_current_phase = MigrationPhase.FAILED
	migration_phase_changed.emit(_current_phase)
	migration_failed.emit(reason)

	# Reset after delay
	await get_tree().create_timer(2.0).timeout
	_current_phase = MigrationPhase.IDLE

## Get current phase
func get_current_phase() -> int:
	return _current_phase

## Get migration progress (0-1)
func get_migration_progress() -> float:
	match _current_phase:
		MigrationPhase.IDLE:
			return 0.0
		MigrationPhase.DETECTING:
			return 0.1
		MigrationPhase.SELECTING_HOST:
			return 0.3
		MigrationPhase.TRANSFERRING_STATE:
			return 0.6
		MigrationPhase.RECONNECTING:
			return 0.8
		MigrationPhase.COMPLETED:
			return 1.0
		MigrationPhase.FAILED:
			return 0.0
		_:
			return 0.0

## Check if migration is in progress
func is_migration_in_progress() -> bool:
	return _current_phase != MigrationPhase.IDLE and \
		   _current_phase != MigrationPhase.COMPLETED and \
		   _current_phase != MigrationPhase.FAILED

## Convert reason to string
func _reason_to_string(reason: int) -> String:
	match reason:
		MigrationReason.HOST_LEFT:
			return "Host Left"
		MigrationReason.HOST_TIMEOUT:
			return "Host Timeout"
		MigrationReason.QUALITY_DEGRADED:
			return "Quality Degraded"
		MigrationReason.MANUAL_REQUEST:
			return "Manual Request"
		_:
			return "Unknown"

## Convert phase to string
static func phase_to_string(phase: int) -> String:
	match phase:
		MigrationPhase.IDLE:
			return "Idle"
		MigrationPhase.DETECTING:
			return "Detecting"
		MigrationPhase.SELECTING_HOST:
			return "Selecting Host"
		MigrationPhase.TRANSFERRING_STATE:
			return "Transferring State"
		MigrationPhase.RECONNECTING:
			return "Reconnecting"
		MigrationPhase.COMPLETED:
			return "Completed"
		MigrationPhase.FAILED:
			return "Failed"
		_:
			return "Unknown"
