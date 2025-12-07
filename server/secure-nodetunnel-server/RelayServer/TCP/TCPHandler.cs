using System.Buffers.Binary;
using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using System.Text;
using NodeTunnel.UDP;
using NodeTunnel.Utils;
using NodeTunnel.Security;

namespace NodeTunnel.TCP;

public class SecureTCPHandler {
    public event Action<string>? PeerDisconnected;
    public event Action<string>? RoomClosed;
    
    private TcpListener _tcp = null!;
    private CancellationTokenSource _ct = null!;
    
    private readonly ConcurrentDictionary<string, Room> _rooms = new();
    private readonly ConcurrentDictionary<string, string> _oidToRid = new();
    private readonly ConcurrentDictionary<TcpClient, string> _tcpToOid = new();
    
    private readonly SecurityLayer _security;

    public SecureTCPHandler(SecurityLayer security) {
        _security = security;
    }
    
    public async Task StartTcpAsync(string host = "0.0.0.0", int port = 9998) {
        _tcp = new TcpListener(IPAddress.Parse(host), port);
        _tcp.Start();
        _ct = new CancellationTokenSource();
        
        Console.WriteLine($"TCP server listening on {host}:{port}");

        while (!_ct.Token.IsCancellationRequested) {
            try {
                var tcpClient = await _tcp.AcceptTcpClientAsync();
                var clientIp = ((IPEndPoint)tcpClient.Client.RemoteEndPoint!).Address;
                
                if (!_security.Connections.AllowConnection(clientIp)) {
                    Console.WriteLine($"[SECURITY] Rejected connection from {clientIp}: connection limit");
                    tcpClient.Close();
                    continue;
                }
                
                if (_security.IsIPBanned(clientIp)) {
                    Console.WriteLine($"[SECURITY] Rejected connection from {clientIp}: banned");
                    _security.Connections.RemoveConnection(clientIp);
                    tcpClient.Close();
                    continue;
                }
                
                _ = Task.Run(() => HandleTcpClient(tcpClient));
            }
            catch (ObjectDisposedException) {
                break;
            }
        }
    }

    private async Task HandleTcpClient(TcpClient client) {
        var buff = new byte[4096];
        var msgBuff = new List<byte>();
        var clientIp = ((IPEndPoint)client.Client.RemoteEndPoint!).Address;

        try {
            var stream = client.GetStream();

            while (client.Connected && !_ct.Token.IsCancellationRequested) {
                var bytes = await stream.ReadAsync(buff, 0, buff.Length);
                if (bytes == 0) {
                    await DisconnectClient(client);
                    break;
                }

                msgBuff.AddRange(buff.Take(bytes));

                while (msgBuff.Count >= 4) {
                    var msgLen = ByteUtils.UnpackU32(msgBuff.ToArray(), 0);

                    if (msgBuff.Count >= 4 + msgLen) {
                        var msgData = msgBuff.Skip(4).Take((int)msgLen).ToArray();
                        msgBuff.RemoveRange(0, 4 + (int)msgLen);

                        var processedData = _security.ProcessIncomingPacket(msgData, clientIp);
                        if (processedData != null) {
                            await HandleTcpMessage(processedData, client);
                        } else {
                            Console.WriteLine($"[SECURITY] Rejected packet from {clientIp}");
                        }
                    }
                    else {
                        break;
                    }
                }
            }
        }
        catch (Exception ex) {
            Console.WriteLine($"TCP Client Error: {ex.Message}");
        }
        finally {
            await DisconnectClient(client);
            _security.Connections.RemoveConnection(clientIp);
            client.Close();
        }
    }

    private async Task DisconnectClient(TcpClient client) {
	    if (!_tcpToOid.TryGetValue(client, out var oid)) return;
        
        Console.WriteLine($"Disconnecting: {oid}");

    	await HandleLeaveRoom(client);
        PeerDisconnected?.Invoke(oid);
        
        var clientIp = ((IPEndPoint)client.Client.RemoteEndPoint!).Address;
        _security.Connections.RemoveConnection(clientIp);
        
        client.Close();
    }

    private async Task SendTcpMessage(TcpClient client, byte[] data) {
        try {
            var stream = client.GetStream();

            var processedData = _security.ProcessOutgoingPacket(data);

            var lenBytes = ByteUtils.PackU32((uint)processedData.Length);
            await stream.WriteAsync(lenBytes, 0, lenBytes.Length);
            await stream.WriteAsync(processedData, 0, processedData.Length);
            await stream.FlushAsync();
        }
        catch (Exception ex) {
            Console.WriteLine($"Error Sending TCP Message: {ex.Message}");
        }
    }
    
    private async Task HandleTcpMessage(byte[] data, TcpClient client) {
        Console.WriteLine("Received Message!");
        var pktType = (PacketType)ByteUtils.UnpackU32(data, 0);
        var payload = data[4..];

        switch (pktType) {
            case PacketType.Connect:
                Console.WriteLine("Received Connect Request");
                await HandleConnect(client);
                break;
            case PacketType.Host:
                Console.WriteLine("Received Host Request");
                await HandleHost(payload, client);
                break;
            case PacketType.Join:
                Console.WriteLine("Received Join Request");
                await HandleJoin(payload, client);
                break;
            case PacketType.PeerList:
                Console.WriteLine("Received Peer List Request");
                break;
            case PacketType.LeaveRoom:
                Console.WriteLine("Leave Room Request");
                await HandleLeaveRoom(client);
                break;
            case PacketType.PromoteToHost:
                Console.WriteLine("Promote To Host Request");
                await HandlePromoteToHost(client);
                break;
            default:
                Console.WriteLine($"Unknown Packet Type: {pktType}");
                break;
        }
    }

    private async Task HandleConnect(TcpClient client) {
        var oid = GenerateOid();
        _tcpToOid[client] = oid;
        
        var clientIp = ((IPEndPoint)client.Client.RemoteEndPoint!).Address;
        var token = _security.Auth.GenerateToken(oid, clientIp);
        
        Console.WriteLine($"OID Generated: {oid}");

        var msg = new List<byte>();
        msg.AddRange(ByteUtils.PackU32((uint)PacketType.Connect));
        msg.AddRange(ByteUtils.PackU32((uint)oid.Length));
        msg.AddRange(Encoding.UTF8.GetBytes(oid));
        
        if (_security.Config.EnableAuthentication) {
            msg.AddRange(ByteUtils.PackU32((uint)token.Length));
            msg.AddRange(Encoding.UTF8.GetBytes(token));
        }
        
        await SendTcpMessage(client, msg.ToArray());
    }
    
    private async Task HandleHost(byte[] data, TcpClient client) {
        var oidLen = ByteUtils.UnpackU32(data, 0);
        var oid = Encoding.UTF8.GetString(data, 4, (int)oidLen);
        
        if (!_security.Validator.ValidateOnlineId(oid)) {
            Console.WriteLine($"[SECURITY] Invalid OID format: {oid}");
            return;
        }
        
        var room = new Room(oid, client);
        _rooms[oid] = room;
	    _oidToRid[oid] = oid;
        
        Console.WriteLine($"Created Room For Peer: {oid}");

        var msg = new List<byte>();
        msg.AddRange(ByteUtils.PackU32((uint)PacketType.Host));

        await SendTcpMessage(client, msg.ToArray());
        await SendPeerList(room);
    }
    
    private async Task HandleJoin(byte[] data, TcpClient client) {
        var oidLen = (int)ByteUtils.UnpackU32(data, 0);
        var oid = Encoding.UTF8.GetString(data, 4, oidLen);

        var hostOidLen = (int)ByteUtils.UnpackU32(data, 4 + oidLen);
        var hostOid = Encoding.UTF8.GetString(data, 8 + oidLen, hostOidLen);

        if (!_security.Validator.ValidateOnlineId(oid) || 
            !_security.Validator.ValidateOnlineId(hostOid)) {
            Console.WriteLine($"[SECURITY] Invalid OID format");
            return;
        }

        if (_rooms.TryGetValue(hostOid, out var room)) {
            room.AddPeer(oid, client);
	        _tcpToOid[client] = oid;
	        _oidToRid[oid] = hostOid;
        }
        else {
            return;
        }

        var msg = new List<byte>();
        msg.AddRange(ByteUtils.PackU32((uint)PacketType.Join));

        await SendTcpMessage(client, msg.ToArray());
        await SendPeerList(room);
    }

    private async Task HandleLeaveRoom(TcpClient client) {
        if (!_tcpToOid.TryGetValue(client, out var oid)) return;

        var room = GetRoomForPeer(oid);
        if (room == null) return;

        // Check if the leaving peer is the current host (use HostOid, not room.Id)
        // room.Id stays the same (original host), but HostOid updates after migration
        if (room.HostOid == oid) {
            Console.WriteLine($"Host {oid} disconnecting, starting migration");

            // Remove host from room but keep room alive
            room.RemovePeer(oid);
            _oidToRid.TryRemove(oid, out _);

            // Start migration process
            room.StartMigration();

            // Start timeout task to close room if migration fails
            _ = Task.Run(async () => {
                await Task.Delay(TimeSpan.FromSeconds(15));

                if (room.HasMigrationTimedOut()) {
                    Console.WriteLine($"[Room {room.Id}] Migration timed out, closing room");

                    // Close the room
                    foreach (var (peerOid, tcpClient) in room.Clients.ToList()) {
                        await SendLeaveRoom(tcpClient);
                        _oidToRid.TryRemove(peerOid, out _);
                    }

                    _rooms.TryRemove(room.Id, out _);
                    RoomClosed?.Invoke(oid);
                }
            });

            // Notify remaining clients to update peer list (host is gone)
            await SendPeerList(room);
        }
        else {
            room.RemovePeer(oid);
	        _oidToRid.TryRemove(oid, out _);
            _ = Task.Run(() => SendPeerList(room));
            _ = Task.Run(() => SendLeaveRoom(client));
        }
    }

    private async Task HandlePromoteToHost(TcpClient client) {
        if (!_tcpToOid.TryGetValue(client, out var newHostOid)) {
            Console.WriteLine("[PromoteToHost] Client not found");
            return;
        }

        var room = GetRoomForPeer(newHostOid);
        if (room == null) {
            Console.WriteLine($"[PromoteToHost] No room found for peer {newHostOid}");
            return;
        }

        if (!room.IsMigrating) {
            Console.WriteLine($"[PromoteToHost] Room {room.Id} is not in migration mode");
            return;
        }

        // Promote the peer to host
        if (room.PromotePeerToHost(newHostOid)) {
            Console.WriteLine($"[PromoteToHost] {newHostOid} is now the host of room {room.Id}");

            // Send updated peer list to all clients (new host will have NID 1)
            await SendPeerList(room);

            // Send success response to new host
            var msg = new List<byte>();
            msg.AddRange(ByteUtils.PackU32((uint)PacketType.PromoteToHost));
            msg.Add(1); // Success
            await SendTcpMessage(client, msg.ToArray());
        } else {
            Console.WriteLine($"[PromoteToHost] Failed to promote {newHostOid}");

            // Send failure response
            var msg = new List<byte>();
            msg.AddRange(ByteUtils.PackU32((uint)PacketType.PromoteToHost));
            msg.Add(0); // Failure
            await SendTcpMessage(client, msg.ToArray());
        }
    }

    private async Task SendPeerList(Room room) {
        Console.WriteLine($"Sending peer list to room: {room.Id}");
        var peers = room.GetPeers();

        var msg = new List<byte>();
        msg.AddRange(ByteUtils.PackU32((uint)PacketType.PeerList));
        msg.AddRange(ByteUtils.PackU32((uint)peers.Count));

        foreach (var (oid, nid) in peers) {
            msg.AddRange(ByteUtils.PackU32((uint)oid.Length));
            msg.AddRange(Encoding.UTF8.GetBytes(oid));
            msg.AddRange(ByteUtils.PackU32((uint)nid));
        }

        foreach (var tcp in room.Clients.Values) {
            Console.WriteLine($"Sending peer list to client: {_tcpToOid[tcp]}");
            await SendTcpMessage(tcp, msg.ToArray());
            Console.WriteLine("Sent peer list!");
        }
    }

    private async Task SendLeaveRoom(TcpClient client) {
        Console.WriteLine($"Sending leave room response to {_tcpToOid[client]}");
        
        var msg = new List<byte>();
        msg.AddRange(ByteUtils.PackU32((uint)PacketType.LeaveRoom));

	if (client.Connected)
	        await SendTcpMessage(client, msg.ToArray());
    }

    public Room? GetRoomForPeer(string oid) {
        return _rooms.Values.FirstOrDefault(room => room.HasPeer(oid));
    }

    private string GenerateOid() {
        const string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        
        using var rng = System.Security.Cryptography.RandomNumberGenerator.Create();
        var bytes = new byte[8];
        rng.GetBytes(bytes);
        
        var oid = new string(bytes.Select(b => chars[b % chars.Length]).ToArray());

        while (_oidToRid.ContainsKey(oid) || _rooms.ContainsKey(oid)) {
            rng.GetBytes(bytes);
            oid = new string(bytes.Select(b => chars[b % chars.Length]).ToArray());
        }

        return oid;
    }

    public int GetTotalRooms() => _rooms.Count;
    public int GetTotalPeers() => _rooms.Values.Sum(room => room.GetPeers().Count);

    public List<(string roomId, int peerCount)> GetRoomDetails() {
        return _rooms.Values.Select(room => (room.Id, room.GetPeers().Count)).ToList();
    }
}
