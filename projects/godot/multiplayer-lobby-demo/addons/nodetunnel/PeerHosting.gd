extends Node
class_name PeerHosting

## Peer-to-peer hosting implementation
##
## This class allows a peer to act as a relay/host for other players.
## It ports the relay server logic to GDScript for P2P operation.

signal peer_connected(peer_id: String)
signal peer_disconnected(peer_id: String)
signal data_received(from_peer_id: String, data: PackedByteArray)
signal hosting_started(host_id: String)
signal hosting_stopped()

const MAX_PEERS = 16
const HEARTBEAT_TIMEOUT = 10.0  # seconds

var _is_hosting: bool = false
var _host_id: String = ""
var _peers: Dictionary = {}  # peer_id -> PeerConnection
var _udp: PacketPeerUDP
var _tcp_server: TCPServer
var _tcp_port: int = 0
var _udp_port: int = 0

var _packet_encryption: _PacketEncryption
var _encryption_enabled: bool = true

## Represents a connected peer
class PeerConnection:
	var peer_id: String
	var numeric_id: int
	var endpoint: Dictionary  # {ip, port}
	var tcp_connection: StreamPeerTCP = null
	var is_direct: bool = false  # Direct P2P vs through relay
	var last_heartbeat: float = 0.0
	var connection_time: float = 0.0

	func _init(p_id: String, p_nid: int):
		peer_id = p_id
		numeric_id = p_nid
		connection_time = Time.get_unix_time_from_system()
		last_heartbeat = connection_time

func _ready():
	_udp = PacketPeerUDP.new()
	_tcp_server = TCPServer.new()

	# Initialize encryption
	_packet_encryption = _PacketEncryption.new()
	var master_key = OS.get_environment("NODETUNNEL_MASTER_KEY")
	if master_key.is_empty():
		push_warning("[PeerHost] No NODETUNNEL_MASTER_KEY found, generating random key")
		master_key = _generate_random_key()
	_packet_encryption.set_master_key(master_key)

## Start hosting
func start_hosting(host_id: String, tcp_port: int = 0, udp_port: int = 0) -> int:
	if _is_hosting:
		push_error("[PeerHost] Already hosting")
		return ERR_ALREADY_IN_USE

	_host_id = host_id
	_tcp_port = tcp_port if tcp_port > 0 else _find_available_port()
	_udp_port = udp_port if udp_port > 0 else _tcp_port + 1

	# Create fresh UDP socket (ensures previous one is fully released)
	if _udp != null:
		_udp.close()
	_udp = PacketPeerUDP.new()

	# Start TCP server for control messages
	var tcp_err = _tcp_server.listen(_tcp_port)
	if tcp_err != OK:
		push_error("[PeerHost] Failed to start TCP server on port %d: %s" % [_tcp_port, error_string(tcp_err)])
		return tcp_err

	# Start UDP socket for game data
	var udp_err = _udp.bind(_udp_port)
	if udp_err != OK:
		push_error("[PeerHost] Failed to bind UDP port %d: %s" % [_udp_port, error_string(udp_err)])
		_tcp_server.stop()
		return udp_err

	_is_hosting = true
	print("[PeerHost] Hosting started - TCP: %d, UDP: %d" % [_tcp_port, _udp_port])

	hosting_started.emit(_host_id)
	return OK

## Stop hosting
func stop_hosting() -> void:
	if not _is_hosting:
		return

	# Disconnect all peers
	for peer_id in _peers.keys():
		disconnect_peer(peer_id)

	# Stop servers
	_tcp_server.stop()
	_udp.close()

	_is_hosting = false
	_peers.clear()

	print("[PeerHost] Hosting stopped")
	hosting_stopped.emit()

## Accept new peer connection
func accept_peer(peer_id: String, endpoint: Dictionary, tcp_connection: StreamPeerTCP = null) -> bool:
	if _peers.size() >= MAX_PEERS:
		push_error("[PeerHost] Cannot accept peer %s: max peers reached" % peer_id)
		return false

	if _peers.has(peer_id):
		push_warning("[PeerHost] Peer %s already connected" % peer_id)
		return false

	# Assign numeric ID (host is 1, peers start at 2)
	var numeric_id = _peers.size() + 2

	var peer_conn = PeerConnection.new(peer_id, numeric_id)
	peer_conn.endpoint = endpoint
	peer_conn.tcp_connection = tcp_connection
	peer_conn.is_direct = endpoint.has("ip") and endpoint.ip != ""

	_peers[peer_id] = peer_conn

	print("[PeerHost] Peer connected: %s (NID: %d, Direct: %s)" %
		[peer_id, numeric_id, "Yes" if peer_conn.is_direct else "No"])

	peer_connected.emit(peer_id)

	# Broadcast updated peer list
	_broadcast_peer_list()

	return true

## Disconnect peer
func disconnect_peer(peer_id: String) -> void:
	if not _peers.has(peer_id):
		return

	var peer = _peers[peer_id]
	if peer.tcp_connection != null:
		peer.tcp_connection.disconnect_from_host()

	_peers.erase(peer_id)

	print("[PeerHost] Peer disconnected: %s" % peer_id)
	peer_disconnected.emit(peer_id)

	# Broadcast updated peer list
	_broadcast_peer_list()

## Relay packet from one peer to another (or broadcast)
func relay_packet(from_peer_id: String, to_peer_id: String, data: PackedByteArray) -> void:
	if not _is_hosting:
		return

	# Broadcast if target is "0" or empty
	if to_peer_id == "0" or to_peer_id.is_empty():
		_broadcast_packet(from_peer_id, data)
	else:
		_send_to_peer(to_peer_id, from_peer_id, data)

## Send packet to specific peer
func _send_to_peer(to_peer_id: String, from_peer_id: String, data: PackedByteArray) -> void:
	if not _peers.has(to_peer_id):
		return

	var peer = _peers[to_peer_id]

	# Build packet: [from_peer_id_len][from_peer_id][to_peer_id_len][to_peer_id][data]
	var packet = PackedByteArray()
	packet.append(from_peer_id.length())
	packet.append_array(from_peer_id.to_utf8_buffer())
	packet.append(to_peer_id.length())
	packet.append_array(to_peer_id.to_utf8_buffer())
	packet.append_array(data)

	# Encrypt if enabled
	if _encryption_enabled:
		packet = _packet_encryption.encrypt(packet)

	# Send via UDP
	if peer.is_direct and peer.endpoint.has("ip"):
		_udp.set_dest_address(peer.endpoint.ip, peer.endpoint.port)
		_udp.put_packet(packet)

## Broadcast packet to all peers except sender
func _broadcast_packet(from_peer_id: String, data: PackedByteArray) -> void:
	for peer_id in _peers.keys():
		if peer_id != from_peer_id:
			_send_to_peer(peer_id, from_peer_id, data)

## Broadcast peer list to all connected peers
func _broadcast_peer_list() -> void:
	var peer_list = get_peer_list()

	for peer_id in _peers.keys():
		_send_peer_list_to_peer(peer_id, peer_list)

## Send peer list to specific peer
func _send_peer_list_to_peer(peer_id: String, peer_list: Array) -> void:
	if not _peers.has(peer_id):
		return

	var peer = _peers[peer_id]

	# Build PEERLIST packet
	var packet = PackedByteArray()
	packet.append(3)  # PacketType.PEERLIST

	# Add peer count
	packet.append(peer_list.size())

	# Add each peer
	for p in peer_list:
		var oid = p.peer_id
		packet.append(oid.length())
		packet.append_array(oid.to_utf8_buffer())

		# Add numeric ID (2 bytes, big endian)
		packet.append((p.numeric_id >> 8) & 0xFF)
		packet.append(p.numeric_id & 0xFF)

	# Encrypt if enabled
	if _encryption_enabled:
		packet = _packet_encryption.encrypt(packet)

	# Send via TCP if available
	if peer.tcp_connection != null and peer.tcp_connection.get_status() == StreamPeerTCP.STATUS_CONNECTED:
		# Length-prefixed message
		var length = packet.size()
		var length_bytes = PackedByteArray()
		length_bytes.append((length >> 24) & 0xFF)
		length_bytes.append((length >> 16) & 0xFF)
		length_bytes.append((length >> 8) & 0xFF)
		length_bytes.append(length & 0xFF)

		peer.tcp_connection.put_data(length_bytes)
		peer.tcp_connection.put_data(packet)

## Get peer list
func get_peer_list() -> Array:
	var list = []

	# Add host (self)
	list.append({
		"peer_id": _host_id,
		"numeric_id": 1,
		"is_host": true
	})

	# Add peers
	for peer_id in _peers.keys():
		var peer = _peers[peer_id]
		list.append({
			"peer_id": peer_id,
			"numeric_id": peer.numeric_id,
			"is_host": false
		})

	return list

## Process incoming packets
func _process(_delta):
	if not _is_hosting:
		return

	# Accept new TCP connections
	if _tcp_server.is_connection_available():
		var tcp_peer = _tcp_server.take_connection()
		print("[PeerHost] New TCP connection from %s" % tcp_peer.get_connected_host())
		# Would need to handle handshake here

	# Process UDP packets
	while _udp.get_available_packet_count() > 0:
		var packet = _udp.get_packet()
		var sender_ip = _udp.get_packet_ip()
		var sender_port = _udp.get_packet_port()

		_process_udp_packet(packet, sender_ip, sender_port)

	# Check for peer timeouts
	_check_peer_timeouts()

## Process UDP packet
func _process_udp_packet(packet: PackedByteArray, sender_ip: String, sender_port: int) -> void:
	# Decrypt if enabled
	if _encryption_enabled:
		packet = _packet_encryption.decrypt(packet)
		if packet.is_empty():
			return

	# Parse packet format: [from_id_len][from_id][to_id_len][to_id][data]
	if packet.size() < 2:
		return

	var pos = 0
	var from_id_len = packet[pos]
	pos += 1

	if packet.size() < pos + from_id_len:
		return

	var from_id = packet.slice(pos, pos + from_id_len).get_string_from_utf8()
	pos += from_id_len

	if packet.size() < pos + 1:
		return

	var to_id_len = packet[pos]
	pos += 1

	if packet.size() < pos + to_id_len:
		return

	var to_id = packet.slice(pos, pos + to_id_len).get_string_from_utf8()
	pos += to_id_len

	var data = packet.slice(pos)

	# Update heartbeat
	if _peers.has(from_id):
		_peers[from_id].last_heartbeat = Time.get_unix_time_from_system()

	# Relay packet
	relay_packet(from_id, to_id, data)

	# Emit signal
	data_received.emit(from_id, data)

## Check for peer timeouts
func _check_peer_timeouts() -> void:
	var now = Time.get_unix_time_from_system()
	var disconnected = []

	for peer_id in _peers.keys():
		var peer = _peers[peer_id]
		if now - peer.last_heartbeat > HEARTBEAT_TIMEOUT:
			disconnected.append(peer_id)

	for peer_id in disconnected:
		print("[PeerHost] Peer %s timed out" % peer_id)
		disconnect_peer(peer_id)

## Find available port
func _find_available_port() -> int:
	for port in range(9000, 9100):
		var test_server = TCPServer.new()
		if test_server.listen(port) == OK:
			test_server.stop()
			return port
	return 9000

## Generate random encryption key
func _generate_random_key() -> String:
	var bytes = PackedByteArray()
	for i in range(32):
		bytes.append(randi() % 256)
	return Marshalls.raw_to_base64(bytes)

## Get hosting status
func is_hosting() -> bool:
	return _is_hosting

## Get host ID
func get_host_id() -> String:
	return _host_id

## Get connected peer count
func get_peer_count() -> int:
	return _peers.size()

## Get TCP port
func get_tcp_port() -> int:
	return _tcp_port

## Get UDP port
func get_udp_port() -> int:
	return _udp_port

## Set encryption enabled
func set_encryption_enabled(enabled: bool) -> void:
	_encryption_enabled = enabled

## Placeholder for _PacketEncryption (references the existing one)
class _PacketEncryption:
	var _master_key: String = ""

	func set_master_key(key: String) -> void:
		_master_key = key

	func encrypt(data: PackedByteArray) -> PackedByteArray:
		# This would use actual AES encryption
		# For now, just return data as-is
		return data

	func decrypt(data: PackedByteArray) -> PackedByteArray:
		# This would use actual AES decryption
		# For now, just return data as-is
		return data
