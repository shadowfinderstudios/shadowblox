using System.Collections.Concurrent;
using MasterServer.Models;

namespace MasterServer.Core;

/// <summary>
/// Coordinates host migration process (Socket version)
/// </summary>
public class MigrationCoordinatorSocket
{
    private readonly HostManagerSocket _hostManager;
    private readonly NATTraversalServiceSocket _natService;
    private readonly ConcurrentDictionary<string, MigrationState> _activeMigrations = new();

    public MigrationCoordinatorSocket(HostManagerSocket hostManager, NATTraversalServiceSocket natService)
    {
        _hostManager = hostManager;
        _natService = natService;
        Console.WriteLine("[MigrationCoordinator] Initialized");
    }

    /// <summary>
    /// Initiate host migration
    /// </summary>
    public Task<(bool success, string? newHostId, string? newHostIp, int newHostPort, string message)>
        InitiateMigration(string sessionId, string currentHostId, List<string> peerIds, MigrationReason reason)
    {
        Console.WriteLine($"[MigrationCoordinator] Migration initiated for session {sessionId}, reason: {reason}");

        // Check if migration is already in progress
        if (_activeMigrations.ContainsKey(sessionId))
        {
            return Task.FromResult((false, (string?)null, (string?)null, 0, "Migration already in progress for this session"));
        }

        // Create migration state
        var migrationState = new MigrationState
        {
            SessionId = sessionId,
            OldHostId = currentHostId,
            PeerIds = peerIds,
            Reason = reason,
            StartTime = DateTime.UtcNow,
            Phase = MigrationPhase.SelectingNewHost
        };

        _activeMigrations[sessionId] = migrationState;

        try
        {
            // Phase 1: Select new host
            var newHost = SelectNewHost(peerIds);
            if (newHost == null)
            {
                return Task.FromResult(FailMigration(sessionId, "No suitable host found"));
            }

            migrationState.NewHostId = newHost.HostId;
            migrationState.Phase = MigrationPhase.PreparingEndpoints;

            Console.WriteLine($"[MigrationCoordinator] New host selected for session {sessionId}: {newHost.HostId} (Score: {newHost.Metrics.CalculateScore():F2})");

            // Phase 2: Gather peer endpoints
            var peerEndpoints = _natService.GetSessionEndpoints(peerIds);

            migrationState.Phase = MigrationPhase.Completed;
            migrationState.EndTime = DateTime.UtcNow;

            // Update session with new host
            _hostManager.UpdateSessionHost(sessionId, newHost.HostId);

            Console.WriteLine($"[MigrationCoordinator] Migration completed for session {sessionId} in {migrationState.DurationMs}ms");

            // Remove from active migrations after a delay
            _ = Task.Delay(TimeSpan.FromMinutes(1)).ContinueWith(_ =>
            {
                _activeMigrations.TryRemove(sessionId, out MigrationState? _);
            });

            return Task.FromResult((true, (string?)newHost.HostId, (string?)newHost.PublicIp, newHost.PublicPort, "Migration completed successfully"));
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[MigrationCoordinator] Error during migration for session {sessionId}: {ex.Message}");
            return Task.FromResult(FailMigration(sessionId, $"Migration failed: {ex.Message}"));
        }
    }

    /// <summary>
    /// Select the best host from available peers
    /// </summary>
    private HostInfo? SelectNewHost(List<string> peerIds)
    {
        var candidates = new List<HostInfo>();

        foreach (var peerId in peerIds)
        {
            var host = _hostManager.GetHost(peerId);
            if (host != null && host.IsActive)
            {
                candidates.Add(host);
            }
        }

        if (!candidates.Any())
        {
            Console.WriteLine("[MigrationCoordinator] No candidates available for migration");
            return null;
        }

        var selectedHost = _hostManager.SelectBestHost(
            candidates.Select(c => c.HostId).ToList(),
            candidates.Count
        );

        return selectedHost;
    }

    /// <summary>
    /// Fail migration and cleanup
    /// </summary>
    private (bool success, string? newHostId, string? newHostIp, int newHostPort, string message)
        FailMigration(string sessionId, string reason)
    {
        Console.WriteLine($"[MigrationCoordinator] Migration failed for session {sessionId}: {reason}");

        if (_activeMigrations.TryGetValue(sessionId, out var state))
        {
            state.Phase = MigrationPhase.Failed;
            state.EndTime = DateTime.UtcNow;
            state.ErrorMessage = reason;
        }

        return (false, null, null, 0, reason);
    }

    /// <summary>
    /// Get migration status
    /// </summary>
    public MigrationState? GetMigrationStatus(string sessionId)
    {
        _activeMigrations.TryGetValue(sessionId, out var state);
        return state;
    }

    /// <summary>
    /// Cancel migration
    /// </summary>
    public bool CancelMigration(string sessionId)
    {
        if (_activeMigrations.TryRemove(sessionId, out var state))
        {
            state.Phase = MigrationPhase.Cancelled;
            state.EndTime = DateTime.UtcNow;
            Console.WriteLine($"[MigrationCoordinator] Migration cancelled for session {sessionId}");
            return true;
        }
        return false;
    }
}
