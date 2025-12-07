using System.Collections.Concurrent;
using MasterServer.Models;

namespace MasterServer.Core;

/// <summary>
/// Coordinates UDP hole punching between peers (Socket version)
/// </summary>
public class NATTraversalServiceSocket
{
    private readonly ConcurrentDictionary<string, PeerEndpoint> _peerEndpoints = new();

    public NATTraversalServiceSocket()
    {
        Console.WriteLine("[NATTraversal] Initialized");
    }

    /// <summary>
    /// Register a peer's endpoint information
    /// </summary>
    public void RegisterPeerEndpoint(string peerId, string publicIp, int publicPort, string privateIp, int privatePort)
    {
        var endpoint = new PeerEndpoint
        {
            PeerId = peerId,
            PublicIp = publicIp,
            PublicPort = publicPort,
            PrivateIp = privateIp,
            PrivatePort = privatePort
        };

        _peerEndpoints[peerId] = endpoint;
        Console.WriteLine($"[NATTraversal] Peer endpoint registered: {peerId} - Public: {publicIp}:{publicPort}, Private: {privateIp}:{privatePort}");
    }

    /// <summary>
    /// Get peer endpoint information
    /// </summary>
    public PeerEndpoint? GetPeerEndpoint(string peerId)
    {
        _peerEndpoints.TryGetValue(peerId, out var endpoint);
        return endpoint;
    }

    /// <summary>
    /// Request hole punching coordination between two peers
    /// </summary>
    public (bool success, string targetPublicIp, int targetPublicPort, string targetPrivateIp, int targetPrivatePort, string message)
        RequestHolePunch(string requesterId, string targetId)
    {
        if (!_peerEndpoints.TryGetValue(targetId, out var targetEndpoint))
        {
            return (false, "", 0, "", 0, "Target peer not found");
        }

        if (!_peerEndpoints.TryGetValue(requesterId, out var requesterEndpoint))
        {
            return (false, "", 0, "", 0, "Requester peer not found");
        }

        Console.WriteLine($"[NATTraversal] Hole punch requested: {requesterId} -> {targetId}");

        return (true, targetEndpoint.PublicIp, targetEndpoint.PublicPort,
                targetEndpoint.PrivateIp, targetEndpoint.PrivatePort, "Hole punch initiated");
    }

    /// <summary>
    /// Report successful connection establishment
    /// </summary>
    public bool ReportConnectionSuccess(string peerId, string targetId)
    {
        Console.WriteLine($"[NATTraversal] Connection established: {peerId} <-> {targetId}");
        return true;
    }

    /// <summary>
    /// Get all peer endpoints for a session
    /// </summary>
    public List<PeerEndpoint> GetSessionEndpoints(List<string> peerIds)
    {
        return peerIds
            .Select(id => GetPeerEndpoint(id))
            .Where(ep => ep != null)
            .Cast<PeerEndpoint>()
            .ToList();
    }

    /// <summary>
    /// Remove peer endpoint
    /// </summary>
    public bool RemovePeerEndpoint(string peerId)
    {
        if (_peerEndpoints.TryRemove(peerId, out _))
        {
            Console.WriteLine($"[NATTraversal] Peer endpoint removed: {peerId}");
            return true;
        }
        return false;
    }
}
