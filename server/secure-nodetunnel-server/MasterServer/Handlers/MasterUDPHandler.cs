using System.Net;
using System.Net.Sockets;
using MasterServer.Core;
using MasterServer.Models;
using MasterServer.Utils;

namespace MasterServer.Handlers;

/// <summary>
/// UDP handler for NAT traversal coordination
/// </summary>
public class MasterUDPHandler
{
    private readonly NATTraversalServiceSocket _natService;
    private readonly UdpClient _udpClient;
    private readonly int _port;

    public MasterUDPHandler(NATTraversalServiceSocket natService, int port = 9051)
    {
        _natService = natService;
        _port = port;
        _udpClient = new UdpClient(port);
    }

    public async Task StartAsync()
    {
        Console.WriteLine($"[MasterUDP] Listening on port {_port}");

        while (true)
        {
            try
            {
                var result = await _udpClient.ReceiveAsync();
                _ = Task.Run(() => ProcessPacket(result.Buffer, result.RemoteEndPoint));
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[MasterUDP] Error receiving packet: {ex.Message}");
            }
        }
    }

    private async Task ProcessPacket(byte[] data, IPEndPoint remoteEndPoint)
    {
        if (data.Length < 1) return;

        var packetType = (MasterPacketType)data[0];
        var offset = 1;

        try
        {
            byte[]? response = packetType switch
            {
                MasterPacketType.RegisterEndpoint => HandleRegisterEndpoint(data, ref offset, remoteEndPoint),
                MasterPacketType.RequestHolePunch => HandleRequestHolePunch(data, ref offset),
                MasterPacketType.ReportSuccess => HandleReportSuccess(data, ref offset),
                _ => null
            };

            if (response != null)
            {
                await _udpClient.SendAsync(response, response.Length, remoteEndPoint);
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[MasterUDP] Error processing {packetType}: {ex.Message}");
        }
    }

    private byte[] HandleRegisterEndpoint(byte[] data, ref int offset, IPEndPoint remoteEndPoint)
    {
        var peerId = PacketSerializer.ReadString(data, ref offset);
        var privateIp = PacketSerializer.ReadString(data, ref offset);
        var privatePort = PacketSerializer.ReadInt32(data, ref offset);

        var publicIp = remoteEndPoint.Address.ToString();
        var publicPort = remoteEndPoint.Port;

        _natService.RegisterPeerEndpoint(peerId, publicIp, publicPort, privateIp, privatePort);

        var response = new List<byte> { (byte)MasterPacketType.RegisterEndpointResponse };
        PacketSerializer.WriteBool(response, true);
        PacketSerializer.WriteString(response, publicIp);
        PacketSerializer.WriteInt32(response, publicPort);

        return response.ToArray();
    }

    private byte[] HandleRequestHolePunch(byte[] data, ref int offset)
    {
        var peerId = PacketSerializer.ReadString(data, ref offset);
        var targetPeerId = PacketSerializer.ReadString(data, ref offset);

        var result = _natService.RequestHolePunch(peerId, targetPeerId);

        var response = new List<byte> { (byte)MasterPacketType.HolePunchResponse };
        PacketSerializer.WriteBool(response, result.success);

        if (result.success)
        {
            PacketSerializer.WriteString(response, result.targetPublicIp);
            PacketSerializer.WriteInt32(response, result.targetPublicPort);
            PacketSerializer.WriteString(response, result.targetPrivateIp);
            PacketSerializer.WriteInt32(response, result.targetPrivatePort);
            PacketSerializer.WriteInt32(response, (int)DateTimeOffset.UtcNow.ToUnixTimeMilliseconds());
        }
        else
        {
            PacketSerializer.WriteString(response, result.message);
        }

        return response.ToArray();
    }

    private byte[] HandleReportSuccess(byte[] data, ref int offset)
    {
        var peerId = PacketSerializer.ReadString(data, ref offset);
        var targetPeerId = PacketSerializer.ReadString(data, ref offset);

        var success = _natService.ReportConnectionSuccess(peerId, targetPeerId);

        var response = new List<byte> { (byte)MasterPacketType.ReportSuccessResponse };
        PacketSerializer.WriteBool(response, success);

        return response.ToArray();
    }
}
