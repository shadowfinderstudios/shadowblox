namespace MasterServer.Models;

/// <summary>
/// Represents a game host with connection information and quality metrics
/// </summary>
public class HostInfo
{
    /// <summary>
    /// Unique identifier for this host
    /// </summary>
    public string HostId { get; set; } = string.Empty;

    /// <summary>
    /// Online ID from the relay server (if applicable)
    /// </summary>
    public string OnlineId { get; set; } = string.Empty;

    /// <summary>
    /// Public IP address of the host
    /// </summary>
    public string PublicIp { get; set; } = string.Empty;

    /// <summary>
    /// Public UDP port for P2P connections
    /// </summary>
    public int PublicPort { get; set; }

    /// <summary>
    /// Private/local IP address (for LAN detection)
    /// </summary>
    public string PrivateIp { get; set; } = string.Empty;

    /// <summary>
    /// Private/local UDP port
    /// </summary>
    public int PrivatePort { get; set; }

    /// <summary>
    /// Game session name/title
    /// </summary>
    public string GameName { get; set; } = string.Empty;

    /// <summary>
    /// Game mode (e.g., "Deathmatch", "Capture the Flag")
    /// </summary>
    public string GameMode { get; set; } = string.Empty;

    /// <summary>
    /// Map name
    /// </summary>
    public string MapName { get; set; } = string.Empty;

    /// <summary>
    /// Current number of players in the session
    /// </summary>
    public int CurrentPlayers { get; set; }

    /// <summary>
    /// Maximum number of players allowed
    /// </summary>
    public int MaxPlayers { get; set; }

    /// <summary>
    /// Whether the session is password protected
    /// </summary>
    public bool IsPasswordProtected { get; set; }

    /// <summary>
    /// Game version (for compatibility checking)
    /// </summary>
    public string GameVersion { get; set; } = string.Empty;

    /// <summary>
    /// Custom metadata (JSON string)
    /// </summary>
    public string CustomData { get; set; } = string.Empty;

    /// <summary>
    /// Connection quality metrics
    /// </summary>
    public ConnectionMetrics Metrics { get; set; } = new();

    /// <summary>
    /// Last heartbeat timestamp
    /// </summary>
    public DateTime LastHeartbeat { get; set; } = DateTime.UtcNow;

    /// <summary>
    /// Host registration timestamp
    /// </summary>
    public DateTime RegisteredAt { get; set; } = DateTime.UtcNow;

    /// <summary>
    /// Whether this host is currently active
    /// </summary>
    public bool IsActive { get; set; } = true;

    /// <summary>
    /// NAT type detected for this host
    /// </summary>
    public NATType NatType { get; set; } = NATType.Unknown;

    /// <summary>
    /// Whether this is a potential host (client registered for migration) vs actual lobby host
    /// </summary>
    public bool IsPotentialHost { get; set; } = false;
}

/// <summary>
/// Connection quality metrics for host selection
/// </summary>
public class ConnectionMetrics
{
    /// <summary>
    /// Average ping to master server in milliseconds
    /// </summary>
    public double PingMs { get; set; }

    /// <summary>
    /// Packet loss percentage (0-100)
    /// </summary>
    public double PacketLossPercent { get; set; }

    /// <summary>
    /// Jitter (ping variance) in milliseconds
    /// </summary>
    public double JitterMs { get; set; }

    /// <summary>
    /// Connection stability score (0-1)
    /// </summary>
    public double StabilityScore { get; set; } = 1.0;

    /// <summary>
    /// CPU usage percentage
    /// </summary>
    public double CpuUsagePercent { get; set; }

    /// <summary>
    /// Available memory in MB
    /// </summary>
    public double AvailableMemoryMB { get; set; }

    /// <summary>
    /// Calculate overall host quality score
    /// </summary>
    public double CalculateScore()
    {
        // Normalize ping (0-1, lower is better, cap at 300ms)
        double normalizedPing = Math.Max(1.0 - (PingMs / 300.0), 0.0);

        // Packet loss penalty (0-1)
        double packetLossPenalty = Math.Max(1.0 - (PacketLossPercent / 10.0), 0.0);

        // Combine metrics with weights
        return 0.25 +
               (normalizedPing * 0.3) +
               (StabilityScore * 0.15) +
               (packetLossPenalty * 0.05);
    }

    /// <summary>
    /// Check if metrics meet minimum hosting requirements
    /// </summary>
    public bool MeetsMinimumRequirements(int playerCount)
    {
        return PingMs < 150 &&
               PacketLossPercent < 5.0 &&
               StabilityScore > 0.95;
    }
}

/// <summary>
/// NAT type classification
/// </summary>
public enum NATType
{
    Unknown,
    Open,           // No NAT, direct connections work
    Moderate,       // Full Cone NAT, easy hole punching
    Strict,         // Symmetric NAT, difficult hole punching
    Blocked         // Cannot accept incoming connections
}

/// <summary>
/// Request to register a new host
/// </summary>
public class RegisterHostRequest
{
    public string HostId { get; set; } = string.Empty;
    public string OnlineId { get; set; } = string.Empty;
    public string PrivateIp { get; set; } = string.Empty;
    public int PrivatePort { get; set; }
    public string GameName { get; set; } = string.Empty;
    public string GameMode { get; set; } = string.Empty;
    public string MapName { get; set; } = string.Empty;
    public int MaxPlayers { get; set; }
    public bool IsPasswordProtected { get; set; }
    public string GameVersion { get; set; } = string.Empty;
    public string CustomData { get; set; } = string.Empty;
    public bool IsPotentialHost { get; set; } = false;
}

/// <summary>
/// Request to update host connection metrics
/// </summary>
public class UpdateMetricsRequest
{
    public string HostId { get; set; } = string.Empty;
    public int CurrentPlayers { get; set; }
    public ConnectionMetrics Metrics { get; set; } = new();
}

/// <summary>
/// Request to initiate host migration
/// </summary>
public class InitiateMigrationRequest
{
    public string CurrentHostId { get; set; } = string.Empty;
    public string SessionId { get; set; } = string.Empty;
    public List<string> PeerIds { get; set; } = new();
    public MigrationReason Reason { get; set; }
}

/// <summary>
/// Reason for host migration
/// </summary>
public enum MigrationReason
{
    HostLeft,
    HostTimeout,
    QualityDegraded,
    ManualRequest
}

/// <summary>
/// Migration phases
/// </summary>
public enum MigrationPhase
{
    SelectingNewHost,
    PreparingEndpoints,
    NotifyingPeers,
    TransferringState,
    EstablishingConnections,
    Completed,
    Failed,
    Cancelled
}

/// <summary>
/// Represents the state of an ongoing migration
/// </summary>
public class MigrationState
{
    public string SessionId { get; set; } = string.Empty;
    public string OldHostId { get; set; } = string.Empty;
    public string NewHostId { get; set; } = string.Empty;
    public List<string> PeerIds { get; set; } = new();
    public MigrationReason Reason { get; set; }
    public MigrationPhase Phase { get; set; }
    public DateTime StartTime { get; set; }
    public DateTime? EndTime { get; set; }
    public string ErrorMessage { get; set; } = string.Empty;

    public double DurationMs => EndTime.HasValue
        ? (EndTime.Value - StartTime).TotalMilliseconds
        : (DateTime.UtcNow - StartTime).TotalMilliseconds;
}

/// <summary>
/// Response for migration request
/// </summary>
public class MigrationResponse
{
    public bool Success { get; set; }
    public string NewHostId { get; set; } = string.Empty;
    public string NewHostIp { get; set; } = string.Empty;
    public int NewHostPort { get; set; }
    public string Message { get; set; } = string.Empty;
    public List<PeerEndpoint> PeerEndpoints { get; set; } = new();
}

/// <summary>
/// Peer endpoint information for reconnection
/// </summary>
public class PeerEndpoint
{
    public string PeerId { get; set; } = string.Empty;
    public string PublicIp { get; set; } = string.Empty;
    public int PublicPort { get; set; }
    public string PrivateIp { get; set; } = string.Empty;
    public int PrivatePort { get; set; }
}

/// <summary>
/// Hole punching coordination request
/// </summary>
public class HolePunchRequest
{
    public string PeerId { get; set; } = string.Empty;
    public string TargetPeerId { get; set; } = string.Empty;
    public string PrivateIp { get; set; } = string.Empty;
    public int PrivatePort { get; set; }
}

/// <summary>
/// Hole punching coordination response
/// </summary>
public class HolePunchResponse
{
    public bool Success { get; set; }
    public string TargetPublicIp { get; set; } = string.Empty;
    public int TargetPublicPort { get; set; }
    public string TargetPrivateIp { get; set; } = string.Empty;
    public int TargetPrivatePort { get; set; }
    public long Timestamp { get; set; }
    public string Message { get; set; } = string.Empty;
}
