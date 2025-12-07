extends NodeTunnelPeer
class_name NodeTunnelPeerP2P

## Enhanced NodeTunnelPeer with P2P support, host migration, and fallback relay
##
## This class extends the base NodeTunnelPeer with:
## - UDP hole punching for direct P2P connections
## - Automatic host migration when host leaves
## - Master server coordination for host selection
## - Fallback to relay server when P2P fails
## - Connection quality measurement for optimal host selection

# P2P-specific signals
signal p2p_connection_established(peer_id: String)
signal p2p_connection_failed(peer_id: String)
signal host_migration_started(reason: String)
signal host_migration_completed(new_host_id: String)
signal host_migration_failed(reason: String)
signal connection_quality_updated(metrics: Dictionary)

enum ConnectionMode {
	RELAY_ONLY,      # Use relay server exclusively (legacy mode)
	P2P_PREFERRED,   # Try P2P first, fallback to relay
	P2P_ONLY         # P2P only, fail if not possible
}

# P2P configuration
var p2p_enabled: bool = true
var connection_mode: int = ConnectionMode.P2P_PREFERRED
var enable_host_migration: bool = true  # If false, lobby closes when host leaves
var master_server_url: String = ""
var fallback_relay_host: String = ""
var fallback_relay_port: int = 0

# P2P components
var _nat_traversal: NAT_Traversal = null
var _peer_hosting: PeerHosting = null
var _host_migration: HostMigration = null
var _connection_quality: ConnectionQuality = null
var _parent_node: Node = null  # Node context for add_child() and get_tree()

# P2P state
var _p2p_connections: Dictionary = {}  # peer_id -> connection status
var _using_fallback: bool = false
var _session_id: String = ""

func _init():
	super._init()

	# NOTE: P2P components must be initialized externally by calling setup_p2p_components()
	# This is because MultiplayerPeerExtension cannot manage child nodes directly

# ============================================================================
# P2P INITIALIZATION
# ============================================================================

## Setup P2P components (must be called from a Node context)
## parent_node: A Node that can manage the P2P component children
func setup_p2p_components(parent_node: Node) -> void:
	if _nat_traversal != null:
		return  # Already initialized

	_parent_node = parent_node

	# Initialize P2P components
	_nat_traversal = NAT_Traversal.new()
	_peer_hosting = PeerHosting.new()
	_host_migration = HostMigration.new()
	_connection_quality = ConnectionQuality.new()

	# Add as children to parent node
	_parent_node.add_child(_nat_traversal)
	_parent_node.add_child(_peer_hosting)
	_parent_node.add_child(_host_migration)
	_parent_node.add_child(_connection_quality)

	# Connect signals
	_nat_traversal.hole_punch_succeeded.connect(_on_hole_punch_succeeded)
	_nat_traversal.hole_punch_failed.connect(_on_hole_punch_failed)
	_peer_hosting.peer_connected.connect(_on_peer_connected_to_host)
	_peer_hosting.peer_disconnected.connect(_on_peer_disconnected_from_host)
	_host_migration.migration_started.connect(_on_migration_started)
	_host_migration.migration_completed.connect(_on_migration_completed)
	_host_migration.migration_failed.connect(_on_migration_failed)
	_connection_quality.metrics_updated.connect(_on_metrics_updated)

	# Connect to base peer_disconnected to detect host disconnect
	peer_disconnected.connect(_on_peer_disconnected_base)

	# Connect to peer_list_updated to detect current host after joining
	peer_list_updated.connect(_on_peer_list_updated)

	print("[P2P] Components initialized")

# ============================================================================
# P2P CONFIGURATION
# ============================================================================

## Enable P2P mode with master server coordination
func set_p2p_enabled(enabled: bool, master_url: String = "") -> void:
	p2p_enabled = enabled
	if enabled and not master_url.is_empty():
		master_server_url = master_url
		print("[P2P] P2P mode enabled with master server: " + master_url)
	else:
		print("[P2P] P2P mode disabled, using relay only")

## Set fallback relay server
func set_fallback_relay(host: String, tcp_port: int, udp_port: int) -> void:
	fallback_relay_host = host
	fallback_relay_port = tcp_port
	print("[P2P] Fallback relay set: %s:%d" % [host, tcp_port])

## Set connection mode preference
func set_connection_mode(mode: int) -> void:
	connection_mode = mode
	match mode:
		ConnectionMode.RELAY_ONLY:
			print("[P2P] Connection mode: RELAY_ONLY")
		ConnectionMode.P2P_PREFERRED:
			print("[P2P] Connection mode: P2P_PREFERRED")
		ConnectionMode.P2P_ONLY:
			print("[P2P] Connection mode: P2P_ONLY")

# ============================================================================
# ENHANCED CONNECTION FLOW
# ============================================================================

## Enhanced connect that also registers with master server
func connect_to_master_and_relay(master_url: String, relay_host: String, relay_port: int) -> void:
	master_server_url = master_url

	# First, connect to relay (always needed for fallback)
	await connect_to_relay(relay_host, relay_port)

	# Register with master server
	await _register_with_master_server()

	# Initialize NAT traversal
	_nat_traversal.initialize(master_server_url, online_id, _get_private_ip(), _get_private_port())

	# Start quality monitoring
	_connection_quality.start_continuous_monitoring(master_server_url, 30.0)

	print("[P2P] Connected to master server and relay")

## Enhanced host that registers as P2P host
func host_p2p() -> void:
	if p2p_enabled:
		# First, set up relay hosting (needed for fallback and multiplayer protocol)
		await host()

		if connection_state != ConnectionState.HOSTING:
			push_error("[P2P] Failed to host via relay")
			return

		# Now add P2P layer on top
		var port = _get_private_port()
		var err = _peer_hosting.start_hosting(online_id, port, port + 1)

		if err != OK:
			push_error("[P2P] Failed to start P2P hosting: %s" % error_string(err))
			# Continue anyway - relay hosting is already active
			print("[P2P] Continuing with relay-only mode")
			return

		_session_id = _generate_session_id()

		# Register as host with master server
		await _register_as_host()

		# Initialize host migration
		_host_migration.initialize(master_server_url, _session_id, online_id)
		_host_migration.set_current_host(online_id, true)

		print("[P2P] P2P hosting enabled (with relay fallback)")

	else:
		# Regular relay hosting
		await host()

## Enhanced join that attempts P2P connection
func join_p2p(host_oid: String) -> void:
	if not p2p_enabled:
		# Regular relay join
		await join(host_oid)
		return

	print("[P2P] Attempting to join via P2P: " + host_oid)

	# Try to establish P2P connection
	if connection_mode != ConnectionMode.RELAY_ONLY:
		_nat_traversal.request_hole_punch(host_oid)
		var connected = await _wait_for_p2p_connection(host_oid, 3.0)

		if connected:
			print("[P2P] Connected via P2P!")
			connection_state = ConnectionState.JOINED
			connection_status = MultiplayerPeer.CONNECTION_CONNECTED

			# Initialize host migration for client - use HOST's online_id for session
			# This ensures all clients in the same session have matching session IDs
			_session_id = _generate_session_id_for_host(host_oid)
			_host_migration.initialize(master_server_url, _session_id, online_id)
			_host_migration.set_current_host(host_oid, false)  # We're not the host

			# Register this client as a potential host for migration (after session ID is set)
			await _register_as_potential_host()

			joined.emit()
			return

	# Fallback to relay if configured
	if connection_mode == ConnectionMode.P2P_PREFERRED:
		print("[P2P] P2P failed, falling back to relay")
		_using_fallback = true

		# Clean up P2P attempts before relay join
		if _nat_traversal != null:
			print("[P2P] Cleaning up NAT traversal before relay fallback")
			_nat_traversal.cleanup()

		# Important: Use the base relay join, not P2P features
		# Just call join() directly - relay connection already established
		await join(host_oid)

		# Initialize host migration for client - use HOST's online_id for session
		# This ensures all clients in the same session have matching session IDs
		_session_id = _generate_session_id_for_host(host_oid)
		_host_migration.initialize(master_server_url, _session_id, online_id)
		_host_migration.set_current_host(host_oid, false)  # We're not the host

		# Register this client as a potential host for migration (after session ID is set)
		await _register_as_potential_host()

		# Join completed via relay
		print("[P2P] Joined via relay fallback")
	else:
		push_error("[P2P] P2P connection failed and no fallback configured")

# ============================================================================
# HOST MIGRATION
# ============================================================================

## Manually trigger host migration
func trigger_host_migration(reason: int = HostMigration.MigrationReason.MANUAL_REQUEST) -> void:
	if connection_state != ConnectionState.JOINED and connection_state != ConnectionState.HOSTING:
		push_error("[P2P] Must be in a session to trigger migration")
		return

	_host_migration.initiate_migration(reason)

## Called when host leaves gracefully
func _handle_host_departure() -> void:
	if connection_state == ConnectionState.HOSTING:
		# We're the host leaving
		print("[P2P] Host leaving gracefully, triggering migration")

		# Backup state for transfer
		var state = _serialize_game_state()
		# Would send state to backup peer here

		# Stop hosting
		_peer_hosting.stop_hosting()

	else:
		# We're a peer and host left
		print("[P2P] Host departed, starting migration")
		_host_migration.initiate_migration(HostMigration.MigrationReason.HOST_LEFT)

# ============================================================================
# MASTER SERVER COMMUNICATION
# ============================================================================

## Register with master server
func _register_with_master_server() -> void:
	if _parent_node == null:
		push_error("[P2P] Cannot register: no parent node set. Call setup_p2p_components() first")
		return

	var http = HTTPRequest.new()
	_parent_node.add_child(http)

	var metrics = _connection_quality.get_metrics()
	var body = JSON.stringify({
		"peerId": online_id,
		"privateIp": _get_private_ip(),
		"privatePort": _get_private_port()
	})

	var url = master_server_url + "/api/nat/register-endpoint"
	var headers = ["Content-Type: application/json"]

	http.request(url, headers, HTTPClient.METHOD_POST, body)
	await http.request_completed

	http.queue_free()

## Register as host with master server
func _register_as_host() -> void:
	if _parent_node == null:
		push_error("[P2P] Cannot register as host: no parent node set")
		return

	var http = HTTPRequest.new()
	_parent_node.add_child(http)

	var metadata = lobby_metadata.duplicate()
	var body = JSON.stringify({
		"hostId": online_id,
		"onlineId": online_id,
		"sessionId": _session_id,
		"privateIp": _get_private_ip(),
		"privatePort": _peer_hosting.get_udp_port(),
		"gameName": metadata.get("Name", "Unknown"),
		"gameMode": metadata.get("Mode", "Unknown"),
		"mapName": metadata.get("Map", "Unknown"),
		"maxPlayers": metadata.get("MaxPlayers", 4),
		"isPasswordProtected": metadata.get("IsPassworded", false),
		"gameVersion": metadata.get("Version", "1.0"),
		"customData": JSON.stringify(metadata)
	})

	var url = master_server_url + "/api/host/register"
	var headers = ["Content-Type: application/json"]

	http.request(url, headers, HTTPClient.METHOD_POST, body)
	var response = await http.request_completed

	if response[1] == 200:
		print("[P2P] Registered as host with master server")
	else:
		push_error("[P2P] Failed to register as host: HTTP %d" % response[1])

	http.queue_free()

	# Start periodic updates
	_start_host_updates()

## Register as potential host (for migration) when joining as client
func _register_as_potential_host() -> void:
	if _parent_node == null:
		push_error("[P2P] Cannot register as potential host: no parent node set")
		return

	var http = HTTPRequest.new()
	_parent_node.add_child(http)

	# Register with minimal info - this client can become a host during migration
	# Include sessionId so master server can filter by session during migration
	var body = JSON.stringify({
		"hostId": online_id,
		"onlineId": online_id,
		"sessionId": _session_id,
		"privateIp": _get_private_ip(),
		"privatePort": _get_private_port(),
		"gameName": "Client (potential host)",
		"gameMode": "Unknown",
		"mapName": "Unknown",
		"maxPlayers": 4,
		"isPasswordProtected": false,
		"gameVersion": "1.0",
		"customData": "{}",
		"isPotentialHost": true
	})

	var url = master_server_url + "/api/host/register"
	var headers = ["Content-Type: application/json"]

	http.request(url, headers, HTTPClient.METHOD_POST, body)
	var response = await http.request_completed

	if response[1] == 200:
		print("[P2P] Registered as potential migration host with master server")
	else:
		push_error("[P2P] Failed to register as potential host: HTTP %d" % response[1])

	http.queue_free()

	# Start periodic metric updates so we're scored for migration
	_start_host_updates()

## Update host metrics periodically
func _start_host_updates() -> void:
	if _parent_node == null:
		return

	var timer = Timer.new()
	_parent_node.add_child(timer)
	timer.wait_time = 5.0
	timer.timeout.connect(_update_host_metrics)
	timer.start()

## Send metrics update to master server
func _update_host_metrics() -> void:
	# Send updates if we're hosting OR if we're a client registered as potential host
	# (clients need to send heartbeats to stay active for migration)
	if _parent_node == null:
		return

	# Only send if we're in a session (hosting or joined)
	if connection_state != ConnectionState.HOSTING and connection_state != ConnectionState.JOINED:
		return

	var http = HTTPRequest.new()
	_parent_node.add_child(http)

	var metrics = _connection_quality.get_metrics()
	var body = JSON.stringify({
		"hostId": online_id,
		"currentPlayers": connected_peers.size(),  # connected_peers already includes host
		"metrics": metrics
	})

	var url = master_server_url + "/api/host/update-metrics"
	var headers = ["Content-Type: application/json"]

	http.request(url, headers, HTTPClient.METHOD_POST, body)
	await http.request_completed

	http.queue_free()

# ============================================================================
# P2P CONNECTION MANAGEMENT
# ============================================================================

## Wait for P2P connection to establish
func _wait_for_p2p_connection(peer_id: String, timeout: float) -> bool:
	if _nat_traversal == null or _parent_node == null:
		return false

	var start_time = Time.get_ticks_msec()

	while Time.get_ticks_msec() - start_time < timeout * 1000:
		if _nat_traversal.is_peer_connected(peer_id):
			return true
		await _parent_node.get_tree().create_timer(0.1).timeout

	return false

## Handle successful hole punch
func _on_hole_punch_succeeded(peer_id: String, endpoint: Dictionary) -> void:
	print("[P2P] Hole punch succeeded: " + peer_id)
	_p2p_connections[peer_id] = true
	p2p_connection_established.emit(peer_id)

## Handle failed hole punch
func _on_hole_punch_failed(peer_id: String, reason: String) -> void:
	# This is expected in many network configurations, not a warning
	print("[P2P] Direct connection not possible with %s, using relay fallback" % peer_id)
	_p2p_connections[peer_id] = false
	p2p_connection_failed.emit(peer_id)

## Handle peer connected to our host
func _on_peer_connected_to_host(peer_id: String) -> void:
	print("[P2P] Peer connected to our host: " + peer_id)
	# Update host migration with new peer
	var peer_list = _peer_hosting.get_peer_list()
	_host_migration.update_peer_list(peer_list)

## Handle peer disconnected (from base class) - detects host disconnect
func _on_peer_disconnected_base(peer_nid: int) -> void:
	print("[P2P] peer_disconnected signal received for nid=%d (my unique_id=%d, connection_state=%s)" % [peer_nid, unique_id, connection_state])

	# Check if the disconnected peer was the host (nid == 1)
	if peer_nid == 1:
		# Don't trigger migration if we ARE the host (unique_id == 1)
		if unique_id == 1:
			print("[P2P] We are the host leaving, no migration needed")
			return

		# Don't trigger migration if we're not fully connected yet
		if connection_state != ConnectionState.JOINED:
			print("[P2P] Host disconnect detected but we're not fully joined yet (state: %s), ignoring" % connection_state)
			return

		# Don't trigger if migration isn't initialized
		if _host_migration == null:
			print("[P2P] Host disconnect detected but HostMigration component is null, ignoring")
			return

		print("[P2P] Host (nid=%d) disconnected!" % peer_nid)

		# Only trigger migration if we're a client and migration is enabled
		if enable_host_migration:
			print("[P2P] Initiating host migration...")
			# initiate_migration will validate initialization and fail safely if not ready
			_host_migration.initiate_migration(HostMigration.MigrationReason.HOST_LEFT)
		else:
			print("[P2P] Host migration disabled - lobby will close")
			# Leave room since host is gone and migration is disabled
			leave_room()

## Handle peer disconnected from our host
func _on_peer_disconnected_from_host(peer_id: String) -> void:
	print("[P2P] Peer disconnected from our host: " + peer_id)

	# Clean up P2P connection data for this peer
	if _p2p_connections.has(peer_id):
		_p2p_connections.erase(peer_id)

	# Clean up NAT traversal data for disconnected peer
	if _nat_traversal != null:
		_nat_traversal.cleanup_peer(peer_id)

	# Update host migration
	var peer_list = _peer_hosting.get_peer_list()
	_host_migration.update_peer_list(peer_list)

# ============================================================================
# HOST MIGRATION HANDLERS
# ============================================================================

func _on_migration_started(reason: String) -> void:
	print("[P2P] Host migration started: " + reason)
	host_migration_started.emit(reason)

func on_promote_res(success: bool):
	if success:
		print("[P2P] Successfully promoted to host on relay server")
	else:
		push_error("[P2P] Failed to promote to host on relay server")
	_packet_manager.promote_to_host_res.disconnect(on_promote_res)

func _on_migration_completed(new_host_id: String) -> void:
	print("[P2P] Host migration completed, new host: " + new_host_id)

	_host_migration.set_current_host(new_host_id, new_host_id == online_id)

	# Update our state
	if new_host_id == online_id:
		# We're the new host!
		print("[P2P] We are the new host - waiting for relay promotion")

		# If we're using relay fallback or RELAY_ONLY mode, notify the relay server
		if _using_fallback or connection_mode == ConnectionMode.RELAY_ONLY:
			print("[P2P] Notifying relay server of host promotion")

			# Connect to response signal once
			_packet_manager.promote_to_host_res.connect(on_promote_res)

			# Send promotion request to relay server
			_packet_manager.send_promote_to_host(_tcp_handler.tcp)

			print("[P2P] Waiting for updated peer list after promotion...")
			await _packet_manager.peer_list_res

			if unique_id == 1:
				print("[P2P] Successfully promoted - unique_id is now 1")
				connection_state = ConnectionState.HOSTING
			else:
				push_error("[P2P] Timeout waiting for relay promotion - unique_id still " + str(unique_id))
		else:
			# P2P mode - start hosting as before
			connection_state = ConnectionState.HOSTING
			var port = _get_private_port()
			_peer_hosting.start_hosting(online_id, port, port + 1)

	host_migration_completed.emit(new_host_id)

func _on_migration_failed(reason: String) -> void:
	push_error("[P2P] Host migration failed: " + reason)
	host_migration_failed.emit(reason)

func _on_metrics_updated(metrics: Dictionary) -> void:
	connection_quality_updated.emit(metrics)

## Handle peer list updates to detect actual current host
func _on_peer_list_updated(nid_to_oid: Dictionary) -> void:
	# Only update host migration state if we're a client (not hosting)
	if connection_state == ConnectionState.JOINED and enable_host_migration:
		# Find who has NID 1 (the actual current host)
		if nid_to_oid.has(1):
			var actual_host_id = nid_to_oid[1]
			# Update host migration state with the ACTUAL current host
			_host_migration.set_current_host(actual_host_id, false)
			print("[P2P] Updated current host to: " + actual_host_id + " (detected from NID 1)")

# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

func _get_private_ip() -> String:
	var addresses = IP.get_local_addresses()
	for addr in addresses:
		# Prefer IPv4 private addresses
		if addr.begins_with("192.168.") or addr.begins_with("10.") or addr.begins_with("172."):
			return addr
	return addresses[0] if addresses.size() > 0 else "127.0.0.1"

func _get_private_port() -> int:
	# Return a port in the game's port range
	return 9000 + (randi() % 100)

## Generate session ID for this peer when hosting
## Using just the online_id ensures all clients in the same session have matching session IDs
func _generate_session_id() -> String:
	return "session_%s" % online_id

## Generate session ID based on a specific host's online ID
## Used when joining so client session IDs match the host's session
func _generate_session_id_for_host(host_oid: String) -> String:
	return "session_%s" % host_oid

func _serialize_game_state() -> Dictionary:
	# This should be overridden by the game to serialize actual state
	return {
		"timestamp": Time.get_unix_time_from_system(),
		"host_id": online_id,
		"peers": connected_peers.keys()
	}

# ============================================================================
# ENHANCED PACKET SENDING
# ============================================================================

## Override to use P2P when available
func _send_packet_enhanced(data: PackedByteArray, to_peer: int) -> void:
	var peer_oid = _numeric_to_online_id.get(to_peer, "")

	# Try P2P first if connected
	if _p2p_connections.get(peer_oid, false):
		var err = _nat_traversal.send_to_peer(peer_oid, data)
		if err == OK:
			return

	# Fallback to relay
	if _using_fallback or connection_mode == ConnectionMode.P2P_PREFERRED:
		# Use existing relay mechanism - would need proper implementation
		pass
	else:
		push_error("[P2P] Cannot send packet: no P2P connection and fallback disabled")
