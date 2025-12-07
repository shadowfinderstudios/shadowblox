using System.Net.Sockets;
using System.Text;

namespace NodeTunnel.TCP;

/**
 * Contains data about different rooms
 * Connected peers, host peer, etc.
 */
public class Room {
    public string Id { get; }
    public string HostOid { get; private set; } = string.Empty;

    private readonly Dictionary<string, int> _oidToNid = new();
    public readonly Dictionary<string, TcpClient> Clients = new();
    private int _nextNid = 2;

    // Host migration support
    public bool IsMigrating { get; private set; } = false;
    public DateTime? MigrationStartTime { get; private set; } = null;
    private const int MIGRATION_TIMEOUT_SECONDS = 15; 

    public Room(string id, TcpClient hostClient) {
        Id = id;
        HostOid = id;  // Initialize HostOid to the original host
        _oidToNid[id] = 1;

        Clients[id] = hostClient;
    }

    public int AddPeer(string oid, TcpClient client) {
        Clients[oid] = client;
    
        if (!_oidToNid.ContainsKey(oid)) {
            var nid = _nextNid++;
            _oidToNid[oid] = nid;
            Console.WriteLine($"Added NEW Peer: {oid}({nid})");
            return nid;
        } else {
            Console.WriteLine($"Updated EXISTING Peer: {oid}({_oidToNid[oid]})");
            return _oidToNid[oid];
        }
    }

    public void RemovePeer(string oid) {
        _oidToNid.Remove(oid);
        Clients.Remove(oid);
    }

    public bool HasPeer(string oid) {
        return Clients.ContainsKey(oid);
    }

    public List<(string oid, int nid)> GetPeers() {
        var peers = new List<(string oid, int nid)>();

        foreach (var kvp in _oidToNid) {
            peers.Add((kvp.Key, kvp.Value));
        }

        return peers;
    }

    /// <summary>
    /// Start host migration process
    /// </summary>
    public void StartMigration() {
        IsMigrating = true;
        MigrationStartTime = DateTime.UtcNow;
        Console.WriteLine($"[Room {Id}] Host migration started");
    }

    /// <summary>
    /// Check if migration has timed out
    /// </summary>
    public bool HasMigrationTimedOut() {
        if (!IsMigrating || !MigrationStartTime.HasValue) {
            return false;
        }

        var elapsed = (DateTime.UtcNow - MigrationStartTime.Value).TotalSeconds;
        return elapsed > MIGRATION_TIMEOUT_SECONDS;
    }

    /// <summary>
    /// Promote a peer to become the new host
    /// </summary>
    public bool PromotePeerToHost(string newHostOid) {
        if (!IsMigrating) {
            Console.WriteLine($"[Room {Id}] Cannot promote peer - not in migration mode");
            return false;
        }

        if (!Clients.ContainsKey(newHostOid)) {
            Console.WriteLine($"[Room {Id}] Cannot promote peer {newHostOid} - not in room");
            return false;
        }

        // Change the old host's NID from 1 to a new peer NID
        if (!string.IsNullOrEmpty(HostOid) && _oidToNid.ContainsKey(HostOid)) {
            _oidToNid[HostOid] = _nextNid++;
        }

        // Assign NID 1 to the new host
        _oidToNid[newHostOid] = 1;
        HostOid = newHostOid;

        // Clear migration state
        IsMigrating = false;
        MigrationStartTime = null;

        Console.WriteLine($"[Room {Id}] Successfully promoted {newHostOid} to host");
        return true;
    }

    /// <summary>
    /// Cancel migration and close the room
    /// </summary>
    public void CancelMigration() {
        IsMigrating = false;
        MigrationStartTime = null;
        Console.WriteLine($"[Room {Id}] Host migration cancelled");
    }
}
