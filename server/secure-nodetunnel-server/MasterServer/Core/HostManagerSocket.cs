using System.Collections.Concurrent;
using MasterServer.Models;

namespace MasterServer.Core;

/// <summary>
/// Manages game hosts, their metrics, and host selection (Socket version)
/// </summary>
public class HostManagerSocket
{
    private readonly ConcurrentDictionary<string, HostInfo> _hosts = new();
    private readonly ConcurrentDictionary<string, List<string>> _sessions = new();
    private readonly Timer _cleanupTimer;

    public HostManagerSocket()
    {
        _cleanupTimer = new Timer(CleanupInactiveHosts, null, TimeSpan.FromSeconds(30), TimeSpan.FromSeconds(30));
        Console.WriteLine("[HostManager] Initialized");
    }

    /// <summary>
    /// Register or update a host
    /// </summary>
    public HostInfo RegisterHost(string hostId, string onlineId, string publicIp, int publicPort,
        string privateIp, int privatePort, string gameName, string gameMode, string mapName,
        int maxPlayers, bool isPasswordProtected, string gameVersion, string customData)
    {
        var hostInfo = new HostInfo
        {
            HostId = hostId,
            OnlineId = onlineId,
            PublicIp = publicIp,
            PublicPort = publicPort,
            PrivateIp = privateIp,
            PrivatePort = privatePort,
            GameName = gameName,
            GameMode = gameMode,
            MapName = mapName,
            MaxPlayers = maxPlayers,
            CurrentPlayers = 1,
            IsPasswordProtected = isPasswordProtected,
            GameVersion = gameVersion,
            CustomData = customData,
            LastHeartbeat = DateTime.UtcNow,
            RegisteredAt = DateTime.UtcNow,
            IsActive = true
        };

        return RegisterHost(hostInfo);
    }

    /// <summary>
    /// Register or update a host from HostInfo object
    /// </summary>
    public HostInfo RegisterHost(HostInfo hostInfo)
    {
        hostInfo.LastHeartbeat = DateTime.UtcNow;
        if (hostInfo.RegisteredAt == default)
            hostInfo.RegisteredAt = DateTime.UtcNow;
        hostInfo.IsActive = true;

        _hosts.AddOrUpdate(hostInfo.HostId, hostInfo, (key, existing) =>
        {
            existing.LastHeartbeat = DateTime.UtcNow;
            existing.GameName = hostInfo.GameName;
            existing.GameMode = hostInfo.GameMode;
            existing.MapName = hostInfo.MapName;
            existing.MaxPlayers = hostInfo.MaxPlayers;
            existing.CurrentPlayers = hostInfo.CurrentPlayers;
            existing.IsPasswordProtected = hostInfo.IsPasswordProtected;
            existing.GameVersion = hostInfo.GameVersion;
            existing.CustomData = hostInfo.CustomData;
            existing.PublicIp = hostInfo.PublicIp;
            existing.PublicPort = hostInfo.PublicPort;
            existing.PrivateIp = hostInfo.PrivateIp;
            existing.PrivatePort = hostInfo.PrivatePort;
            existing.IsActive = true;
            return existing;
        });

        Console.WriteLine($"[HostManager] Host registered: {hostInfo.HostId} at {hostInfo.PublicIp}:{hostInfo.PublicPort}");
        return _hosts[hostInfo.HostId];
    }

    /// <summary>
    /// Update host metrics
    /// </summary>
    public bool UpdateMetrics(string hostId, int currentPlayers, ConnectionMetrics metrics)
    {
        if (_hosts.TryGetValue(hostId, out var host))
        {
            host.Metrics = metrics;
            host.CurrentPlayers = currentPlayers;
            host.LastHeartbeat = DateTime.UtcNow;
            return true;
        }
        return false;
    }

    /// <summary>
    /// Send heartbeat to keep host alive
    /// </summary>
    public bool Heartbeat(string hostId)
    {
        if (_hosts.TryGetValue(hostId, out var host))
        {
            host.LastHeartbeat = DateTime.UtcNow;
            host.IsActive = true;
            return true;
        }
        return false;
    }

    /// <summary>
    /// Get all active hosts
    /// </summary>
    public List<HostInfo> GetActiveHosts(string? gameVersion = null, string? gameMode = null, bool? passwordProtected = null)
    {
        IEnumerable<HostInfo> hosts = _hosts.Values
            .Where(h => h.IsActive && h.CurrentPlayers < h.MaxPlayers);

        if (!string.IsNullOrEmpty(gameVersion))
            hosts = hosts.Where(h => h.GameVersion == gameVersion);

        if (!string.IsNullOrEmpty(gameMode))
            hosts = hosts.Where(h => h.GameMode == gameMode);

        if (passwordProtected.HasValue)
            hosts = hosts.Where(h => h.IsPasswordProtected == passwordProtected.Value);

        return hosts.OrderByDescending(h => h.Metrics.CalculateScore()).ToList();
    }

    /// <summary>
    /// Get host by ID
    /// </summary>
    public HostInfo? GetHost(string hostId)
    {
        _hosts.TryGetValue(hostId, out var host);
        return host;
    }

    /// <summary>
    /// Select the best host from a list of candidates
    /// </summary>
    public HostInfo? SelectBestHost(List<string> candidateIds, int requiredPlayerSlots = 1)
    {
        var candidates = candidateIds
            .Select(id => GetHost(id))
            .Where(h => h != null && h.IsActive && h.Metrics.MeetsMinimumRequirements(requiredPlayerSlots))
            .Cast<HostInfo>()
            .OrderByDescending(h => h.Metrics.CalculateScore())
            .ToList();

        if (candidates.Any())
        {
            var selected = candidates.First();
            Console.WriteLine($"[HostManager] Selected host: {selected.HostId} with score {selected.Metrics.CalculateScore():F2}");
            return selected;
        }

        Console.WriteLine($"[HostManager] No suitable host found from {candidateIds.Count} candidates");
        return null;
    }

    /// <summary>
    /// Unregister a host
    /// </summary>
    public bool UnregisterHost(string hostId)
    {
        if (_hosts.TryRemove(hostId, out var host))
        {
            Console.WriteLine($"[HostManager] Host unregistered: {hostId}");
            return true;
        }
        return false;
    }

    /// <summary>
    /// Register a game session
    /// </summary>
    public void RegisterSession(string sessionId, string hostId, List<string> peerIds)
    {
        var allPeers = new List<string> { hostId };
        allPeers.AddRange(peerIds);
        _sessions[sessionId] = allPeers;
        Console.WriteLine($"[HostManager] Session registered: {sessionId} with {allPeers.Count} peers");
    }

    /// <summary>
    /// Get session peers
    /// </summary>
    public List<string>? GetSessionPeers(string sessionId)
    {
        _sessions.TryGetValue(sessionId, out var peers);
        return peers;
    }

    /// <summary>
    /// Update session host after migration
    /// </summary>
    public bool UpdateSessionHost(string sessionId, string newHostId)
    {
        if (_sessions.TryGetValue(sessionId, out var peers))
        {
            if (peers.Contains(newHostId))
            {
                peers.Remove(newHostId);
                peers.Insert(0, newHostId);
                Console.WriteLine($"[HostManager] Session {sessionId} host updated to {newHostId}");
                return true;
            }
        }
        return false;
    }

    /// <summary>
    /// Remove session
    /// </summary>
    public bool RemoveSession(string sessionId)
    {
        if (_sessions.TryRemove(sessionId, out _))
        {
            Console.WriteLine($"[HostManager] Session removed: {sessionId}");
            return true;
        }
        return false;
    }

    /// <summary>
    /// Cleanup inactive hosts
    /// </summary>
    private void CleanupInactiveHosts(object? state)
    {
        var timeout = TimeSpan.FromSeconds(30);
        var now = DateTime.UtcNow;
        var inactiveHosts = _hosts.Values
            .Where(h => now - h.LastHeartbeat > timeout)
            .ToList();

        foreach (var host in inactiveHosts)
        {
            host.IsActive = false;
            Console.WriteLine($"[HostManager] Host marked inactive due to timeout: {host.HostId}");
        }

        // Remove hosts that have been inactive for too long (5 minutes)
        var removalTimeout = TimeSpan.FromMinutes(5);
        var hostsToRemove = _hosts.Values
            .Where(h => !h.IsActive && now - h.LastHeartbeat > removalTimeout)
            .ToList();

        foreach (var host in hostsToRemove)
        {
            _hosts.TryRemove(host.HostId, out _);
            Console.WriteLine($"[HostManager] Host removed after extended inactivity: {host.HostId}");
        }
    }

    /// <summary>
    /// Get statistics
    /// </summary>
    public (int totalHosts, int activeHosts, int totalSessions, int totalPlayers) GetStatistics()
    {
        return (
            _hosts.Count,
            _hosts.Values.Count(h => h.IsActive),
            _sessions.Count,
            _hosts.Values.Sum(h => h.CurrentPlayers)
        );
    }
}
