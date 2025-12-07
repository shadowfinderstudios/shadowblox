extends Node
class_name NAT_Traversal

## UDP Hole Punching implementation for direct P2P connections
##
## This class handles NAT traversal using UDP hole punching technique
## coordinated through the Master Server.

signal hole_punch_succeeded(peer_id: String, endpoint: Dictionary)
signal hole_punch_failed(peer_id: String, reason: String)
signal nat_type_detected(nat_type: int)

enum NATType {
	UNKNOWN,
	OPEN,       # No NAT, direct connections work
	MODERATE,   # Full Cone NAT, easy hole punching
	STRICT,     # Symmetric NAT, difficult hole punching
	BLOCKED     # Cannot accept incoming connections
}

const MAX_PUNCH_ATTEMPTS = 3
const PUNCH_TIMEOUT_MS = 500
const PUNCH_PACKET_SIZE = 64

var _udp: PacketPeerUDP
var _master_server_url: String = ""
var _peer_id: String = ""
var _private_ip: String = ""
var _private_port: int = 0
var _public_ip: String = ""
var _public_port: int = 0
var _detected_nat_type: int = NATType.UNKNOWN

var _active_punches: Dictionary = {} # peer_id -> PunchAttempt
var _established_connections: Dictionary = {} # peer_id -> endpoint info

## Represents an active hole punching attempt
class PunchAttempt:
	var peer_id: String
	var target_endpoint: Dictionary
	var attempts: int = 0
	var start_time: int = 0
	var last_attempt_time: int = 0

	func _init(p_peer_id: String, p_endpoint: Dictionary):
		peer_id = p_peer_id
		target_endpoint = p_endpoint
		start_time = Time.get_ticks_msec()

func _ready():
	_udp = PacketPeerUDP.new()

## Initialize NAT traversal with master server
func initialize(master_url: String, peer_id: String, private_ip: String, private_port: int) -> void:
	_master_server_url = master_url
	_peer_id = peer_id
	_private_ip = private_ip
	_private_port = private_port

	# Bind UDP socket
	var err = _udp.bind(_private_port)
	if err != OK:
		push_error("Failed to bind UDP port %d: %s" % [_private_port, error_string(err)])
		return

	print("[NAT] Initialized on %s:%d" % [_private_ip, _private_port])

	# Register endpoint with master server
	await _register_endpoint()

## Register our endpoint with the master server
func _register_endpoint() -> void:
	var http = HTTPRequest.new()
	add_child(http)

	var body = JSON.stringify({
		"peerId": _peer_id,
		"privateIp": _private_ip,
		"privatePort": _private_port,
		"targetPeerId": ""
	})

	var url = _master_server_url + "/api/nat/register-endpoint"
	var headers = ["Content-Type: application/json"]

	http.request(url, headers, HTTPClient.METHOD_POST, body)
	var response = await http.request_completed

	if response[1] == 200:
		var json = JSON.new()
		var parse_result = json.parse(response[3].get_string_from_utf8())
		if parse_result == OK:
			var data = json.data
			_public_ip = data.get("publicIp", "")
			_public_port = data.get("publicPort", _private_port)
			print("[NAT] Endpoint registered - Public: %s:%d" % [_public_ip, _public_port])
	else:
		push_error("[NAT] Failed to register endpoint: HTTP %d" % response[1])

	http.queue_free()

## Request hole punching to a target peer
func request_hole_punch(target_peer_id: String) -> void:
	if _active_punches.has(target_peer_id):
		print("[NAT] Hole punch already in progress for %s" % target_peer_id)
		return

	if _established_connections.has(target_peer_id):
		print("[NAT] Connection already established to %s" % target_peer_id)
		hole_punch_succeeded.emit(target_peer_id, _established_connections[target_peer_id])
		return

	print("[NAT] Requesting hole punch to %s" % target_peer_id)

	# Request coordination from master server
	var endpoint = await _request_peer_endpoint(target_peer_id)

	if endpoint == null:
		hole_punch_failed.emit(target_peer_id, "Failed to get peer endpoint")
		return

	# Start hole punching attempts
	var attempt = PunchAttempt.new(target_peer_id, endpoint)
	_active_punches[target_peer_id] = attempt

	_perform_hole_punch(attempt)

## Request peer endpoint from master server
func _request_peer_endpoint(target_peer_id: String) -> Dictionary:
	var http = HTTPRequest.new()
	add_child(http)

	var body = JSON.stringify({
		"peerId": _peer_id,
		"targetPeerId": target_peer_id,
		"privateIp": _private_ip,
		"privatePort": _private_port
	})

	var url = _master_server_url + "/api/nat/request-hole-punch"
	var headers = ["Content-Type: application/json"]

	http.request(url, headers, HTTPClient.METHOD_POST, body)
	var response = await http.request_completed

	var result = null
	if response[1] == 200:
		var json = JSON.new()
		var parse_result = json.parse(response[3].get_string_from_utf8())
		if parse_result == OK:
			var data = json.data
			if data.get("success", false):
				result = {
					"public_ip": data.get("targetPublicIp", ""),
					"public_port": data.get("targetPublicPort", 0),
					"private_ip": data.get("targetPrivateIp", ""),
					"private_port": data.get("targetPrivatePort", 0),
					"timestamp": data.get("timestamp", 0)
				}

	http.queue_free()
	return result if result != null else {}

## Perform hole punching attempts
func _perform_hole_punch(attempt: PunchAttempt) -> void:
	var endpoint = attempt.target_endpoint

	# Try both public and private endpoints (for LAN detection)
	var endpoints = [
		{"ip": endpoint.public_ip, "port": endpoint.public_port},
		{"ip": endpoint.private_ip, "port": endpoint.private_port}
	]

	for i in range(MAX_PUNCH_ATTEMPTS):
		attempt.attempts = i + 1
		attempt.last_attempt_time = Time.get_ticks_msec()

		# Send punch packets to all endpoints
		for ep in endpoints:
			_send_punch_packet(ep.ip, ep.port, attempt.peer_id)

		# Wait for response
		await get_tree().create_timer(PUNCH_TIMEOUT_MS / 1000.0).timeout

		# Check if connection established (would be detected in _process)
		if _established_connections.has(attempt.peer_id):
			_active_punches.erase(attempt.peer_id)
			hole_punch_succeeded.emit(attempt.peer_id, _established_connections[attempt.peer_id])
			_report_success(attempt.peer_id)
			return

	# All attempts failed (expected in many network configurations)
	_active_punches.erase(attempt.peer_id)
	hole_punch_failed.emit(attempt.peer_id, "Timeout after %d attempts" % MAX_PUNCH_ATTEMPTS)
	print("[NAT] Direct P2P connection not possible with %s (will use relay)" % attempt.peer_id)

## Send a hole punch packet
func _send_punch_packet(ip: String, port: int, target_peer_id: String) -> void:
	var packet = PackedByteArray()

	# Packet format: [PUNCH_MAGIC:8][peer_id_len:1][peer_id:N][timestamp:8]
	packet.append_array("PUNCH_V1".to_utf8_buffer())
	packet.append(_peer_id.length())
	packet.append_array(_peer_id.to_utf8_buffer())

	var timestamp = Time.get_ticks_msec()
	packet.append_array(_encode_uint64(timestamp))

	# Pad to PUNCH_PACKET_SIZE
	while packet.size() < PUNCH_PACKET_SIZE:
		packet.append(0)

	_udp.set_dest_address(ip, port)
	var err = _udp.put_packet(packet)

	if err == OK:
		print("[NAT] Sent punch packet to %s:%d for peer %s" % [ip, port, target_peer_id])
	else:
		# Don't spam errors - punch failures are expected when P2P doesn't work
		print("[NAT] Could not send punch packet to %s:%d (likely behind strict NAT)" % [ip, port])

## Process incoming UDP packets
func _process(_delta):
	while _udp.get_available_packet_count() > 0:
		var packet = _udp.get_packet()
		var sender_ip = _udp.get_packet_ip()
		var sender_port = _udp.get_packet_port()

		_process_packet(packet, sender_ip, sender_port)

## Process a received packet
func _process_packet(packet: PackedByteArray, sender_ip: String, sender_port: int) -> void:
	if packet.size() < 9:  # Minimum size for valid packet
		return

	# Check magic header
	var magic = packet.slice(0, 8).get_string_from_utf8()
	if magic != "PUNCH_V1":
		return

	# Parse peer ID
	var peer_id_len = packet[8]
	if packet.size() < 9 + peer_id_len:
		return

	var peer_id = packet.slice(9, 9 + peer_id_len).get_string_from_utf8()

	print("[NAT] Received punch packet from %s at %s:%d" % [peer_id, sender_ip, sender_port])

	# Check if this is a response to our punch attempt
	if _active_punches.has(peer_id):
		# Connection established!
		var endpoint = {
			"peer_id": peer_id,
			"ip": sender_ip,
			"port": sender_port
		}
		_established_connections[peer_id] = endpoint
		print("[NAT] Connection established to %s at %s:%d" % [peer_id, sender_ip, sender_port])

	# Always respond to punch packets (for simultaneous punch)
	_send_punch_packet(sender_ip, sender_port, peer_id)

## Report successful connection to master server
func _report_success(peer_id: String) -> void:
	var http = HTTPRequest.new()
	add_child(http)

	var body = JSON.stringify({
		"peerId": _peer_id,
		"targetPeerId": peer_id
	})

	var url = _master_server_url + "/api/nat/report-success"
	var headers = ["Content-Type: application/json"]

	http.request(url, headers, HTTPClient.METHOD_POST, body)
	await http.request_completed

	http.queue_free()

## Get established connection endpoint
func get_peer_endpoint(peer_id: String) -> Dictionary:
	return _established_connections.get(peer_id, {})

## Check if connection is established to a peer
func is_peer_connected(peer_id: String) -> bool:
	return _established_connections.has(peer_id)

## Send data to peer via established connection
func send_to_peer(peer_id: String, data: PackedByteArray) -> int:
	if not _established_connections.has(peer_id):
		return ERR_UNAVAILABLE

	var endpoint = _established_connections[peer_id]
	_udp.set_dest_address(endpoint.ip, endpoint.port)
	return _udp.put_packet(data)

## Detect NAT type based on connection attempts
func detect_nat_type(test_endpoints: Array) -> int:
	# Simple NAT type detection
	# In production, this would involve more sophisticated testing

	var successful_punches = 0
	var total_attempts = test_endpoints.size()

	for endpoint in test_endpoints:
		if await _test_connection(endpoint):
			successful_punches += 1

	var nat_type = NATType.UNKNOWN
	if successful_punches == total_attempts:
		nat_type = NATType.OPEN
	elif successful_punches > 0:
		nat_type = NATType.MODERATE
	elif total_attempts > 0:
		nat_type = NATType.STRICT
	else:
		nat_type = NATType.BLOCKED

	_detected_nat_type = nat_type
	nat_type_detected.emit(nat_type)
	return nat_type

## Test connection to an endpoint
func _test_connection(endpoint: Dictionary) -> bool:
	# Send test packet and wait for response
	_send_punch_packet(endpoint.ip, endpoint.port, "TEST")
	await get_tree().create_timer(1.0).timeout
	return false  # Would be true if response received

## Encode uint64 to bytes (big endian)
func _encode_uint64(value: int) -> PackedByteArray:
	var bytes = PackedByteArray()
	for i in range(8):
		bytes.append((value >> (56 - i * 8)) & 0xFF)
	return bytes

## Clean up specific peer connection
func cleanup_peer(peer_id: String) -> void:
	if _established_connections.has(peer_id):
		_established_connections.erase(peer_id)
	if _active_punches.has(peer_id):
		_active_punches.erase(peer_id)
	print("[NAT] Cleaned up peer: " + peer_id)

## Clean up all connections
func cleanup() -> void:
	_udp.close()
	_active_punches.clear()
	_established_connections.clear()
	print("[NAT] Cleanup completed")
