namespace MasterServer;

/// <summary>
/// Packet types for Master Server communication
/// </summary>
public enum MasterPacketType : byte
{
    // Host Management (TCP)
    RegisterHost = 0,
    RegisterHostResponse = 1,
    UpdateMetrics = 2,
    UpdateMetricsResponse = 3,
    Heartbeat = 4,
    HeartbeatResponse = 5,
    GetHostList = 6,
    GetHostListResponse = 7,
    UnregisterHost = 8,
    UnregisterHostResponse = 9,
    GetStats = 10,
    GetStatsResponse = 11,

    // NAT Traversal (UDP)
    RegisterEndpoint = 20,
    RegisterEndpointResponse = 21,
    RequestHolePunch = 22,
    HolePunchResponse = 23,
    ReportSuccess = 24,
    ReportSuccessResponse = 25,

    // Migration (TCP)
    InitiateMigration = 30,
    MigrationResponse = 31,
    GetMigrationStatus = 32,
    MigrationStatusResponse = 33,
    CancelMigration = 34,
    CancelMigrationResponse = 35,

    // Generic
    Error = 255
}
