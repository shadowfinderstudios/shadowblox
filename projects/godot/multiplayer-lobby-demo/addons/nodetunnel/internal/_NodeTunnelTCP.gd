class_name _NodeTunnelTCP

signal tcp_connected
@warning_ignore("unused_signal")
signal relay_connected(oid: String)

var tcp: StreamPeerTCP
var connected = false
var oid = ""

var _buffer = PackedByteArray()
var _encryption: PacketEncryption = null

func _init() -> void:
	tcp = StreamPeerTCP.new()
	tcp.big_endian = true

func set_encryption(encryption: PacketEncryption) -> void:
	_encryption = encryption

func poll(packet_manager: _PacketManager) -> void:
	tcp.poll()
	
	if tcp.get_status() != StreamPeerTCP.STATUS_CONNECTED:
		return
	
	if connected == false:
		tcp_connected.emit()
		connected = true
	
	var available = tcp.get_available_bytes()
	if available > 0:
		var new_data = tcp.get_data(available)[1]
		_buffer.append_array(new_data)
	
	while _buffer.size() >= 4:
		var msg_len = ByteUtils.unpack_u32(_buffer, 0)
		
		if _buffer.size() >= 4 + msg_len:
			var msg_data = _buffer.slice(4, 4 + msg_len)
			_buffer = _buffer.slice(4 + msg_len)
			
			if _encryption != null and _encryption.is_enabled():
				msg_data = _encryption.decrypt(msg_data)
				if msg_data.is_empty():
					push_error("[NodeTunnel TCP] Failed to decrypt packet")
					continue
			
			packet_manager.handle_msg(msg_data)
		else:
			break
