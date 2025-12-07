using System.Linq;
using System.Net;
using System.Net.Sockets;
using MasterServer.Core;
using MasterServer.Models;
using MasterServer.Utils;

namespace MasterServer.Handlers;

/// <summary>
/// TCP handler for Master Server control messages
/// </summary>
public class MasterTCPHandler
{
    private readonly HostManagerSocket _hostManager;
    private readonly MigrationCoordinatorSocket _migrationCoordinator;
    private readonly TcpListener _listener;
    private readonly int _port;

    public MasterTCPHandler(HostManagerSocket hostManager, MigrationCoordinatorSocket migrationCoordinator, int port = 9050)
    {
        _hostManager = hostManager;
        _migrationCoordinator = migrationCoordinator;
        _port = port;
        _listener = new TcpListener(IPAddress.Any, port);
    }

    public async Task StartAsync()
    {
        _listener.Start();
        Console.WriteLine($"[MasterTCP] Listening on port {_port}");

        while (true)
        {
            var client = await _listener.AcceptTcpClientAsync();
            _ = Task.Run(() => HandleClient(client));
        }
    }

    private async Task HandleClient(TcpClient client)
    {
        var endpoint = client.Client.RemoteEndPoint as IPEndPoint;
        var clientIp = endpoint?.Address.ToString() ?? "unknown";

        var isHttpConnection = false;  // Track if this is HTTP for quieter logging

        try
        {
            var stream = client.GetStream();
            var buffer = new byte[65536];

            while (client.Connected)
            {
                // Peek first few bytes to detect protocol
                var peekBuffer = new byte[4];
                var peeked = await stream.ReadAsync(peekBuffer, 0, 4);
                if (peeked < 4) break;

                // Check if this is HTTP by looking for "GET ", "POST", "PUT ", "DELE"
                var isHttp = (peekBuffer[0] == 'G' && peekBuffer[1] == 'E' && peekBuffer[2] == 'T' && peekBuffer[3] == ' ') ||
                             (peekBuffer[0] == 'P' && peekBuffer[1] == 'O' && peekBuffer[2] == 'S' && peekBuffer[3] == 'T') ||
                             (peekBuffer[0] == 'P' && peekBuffer[1] == 'U' && peekBuffer[2] == 'T' && peekBuffer[3] == ' ') ||
                             (peekBuffer[0] == 'D' && peekBuffer[1] == 'E' && peekBuffer[2] == 'L' && peekBuffer[3] == 'E');

                if (isHttp)
                {
                    // Handle HTTP request (no connect/disconnect logging - it's ephemeral)
                    isHttpConnection = true;
                    await HandleHttpRequest(stream, peekBuffer, client);
                    break; // HTTP is typically one request per connection
                }
                else
                {
                    // Handle binary protocol - read length from the 4 bytes we already read
                    var bytesRead = 0;
                    var length = (int)PacketSerializer.ReadUInt32(peekBuffer, ref bytesRead);
                    if (length <= 0 || length > 65536) break;

                    // Read payload
                    bytesRead = await stream.ReadAsync(buffer, 0, length);
                    if (bytesRead != length) break;

                    // Process packet
                    var response = await ProcessPacket(buffer, length, clientIp, endpoint?.Port ?? 0);

                    // Send response
                    if (response != null)
                    {
                        await stream.WriteAsync(response, 0, response.Length);
                    }
                }
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[MasterTCP] Error handling client {clientIp}: {ex.Message}");
        }
        finally
        {
            client.Close();
            // Only log disconnect for persistent connections (not HTTP which is ephemeral)
            if (!isHttpConnection)
            {
                Console.WriteLine($"[MasterTCP] Client disconnected: {clientIp}");
            }
        }
    }

    private async Task<byte[]?> ProcessPacket(byte[] data, int length, string clientIp, int clientPort)
    {
        var packetType = (MasterPacketType)data[0];
        var offset = 1;

        try
        {
            return packetType switch
            {
                MasterPacketType.RegisterHost => HandleRegisterHost(data, ref offset, clientIp, clientPort),
                MasterPacketType.UpdateMetrics => HandleUpdateMetrics(data, ref offset),
                MasterPacketType.Heartbeat => HandleHeartbeat(data, ref offset),
                MasterPacketType.GetHostList => HandleGetHostList(data, ref offset),
                MasterPacketType.UnregisterHost => HandleUnregisterHost(data, ref offset),
                MasterPacketType.GetStats => HandleGetStats(),
                MasterPacketType.InitiateMigration => await HandleInitiateMigration(data, offset),
                MasterPacketType.GetMigrationStatus => HandleGetMigrationStatus(data, ref offset),
                MasterPacketType.CancelMigration => HandleCancelMigration(data, ref offset),
                _ => CreateErrorResponse($"Unknown packet type: {packetType}")
            };
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[MasterTCP] Error processing {packetType}: {ex.Message}");
            return CreateErrorResponse(ex.Message);
        }
    }

    private byte[] HandleRegisterHost(byte[] data, ref int offset, string clientIp, int clientPort)
    {
        var hostId = PacketSerializer.ReadString(data, ref offset);
        var onlineId = PacketSerializer.ReadString(data, ref offset);
        var privateIp = PacketSerializer.ReadString(data, ref offset);
        var privatePort = PacketSerializer.ReadInt32(data, ref offset);
        var gameName = PacketSerializer.ReadString(data, ref offset);
        var gameMode = PacketSerializer.ReadString(data, ref offset);
        var mapName = PacketSerializer.ReadString(data, ref offset);
        var maxPlayers = PacketSerializer.ReadInt32(data, ref offset);
        var isPasswordProtected = PacketSerializer.ReadBool(data, ref offset);
        var gameVersion = PacketSerializer.ReadString(data, ref offset);
        var customData = PacketSerializer.ReadString(data, ref offset);

        var hostInfo = _hostManager.RegisterHost(
            hostId, onlineId, clientIp, clientPort,
            privateIp, privatePort, gameName, gameMode, mapName,
            maxPlayers, isPasswordProtected, gameVersion, customData);

        // Build response
        var response = new List<byte> { (byte)MasterPacketType.RegisterHostResponse };
        PacketSerializer.WriteBool(response, true);
        PacketSerializer.WriteString(response, "Host registered successfully");
        PacketSerializer.WriteString(response, hostInfo.HostId);

        return PacketSerializer.CreatePacket(response);
    }

    private byte[] HandleUpdateMetrics(byte[] data, ref int offset)
    {
        var hostId = PacketSerializer.ReadString(data, ref offset);
        var currentPlayers = PacketSerializer.ReadInt32(data, ref offset);

        var metrics = new ConnectionMetrics
        {
            PingMs = PacketSerializer.ReadDouble(data, ref offset),
            PacketLossPercent = PacketSerializer.ReadDouble(data, ref offset),
            JitterMs = PacketSerializer.ReadDouble(data, ref offset),
            StabilityScore = PacketSerializer.ReadDouble(data, ref offset),
            CpuUsagePercent = PacketSerializer.ReadDouble(data, ref offset),
            AvailableMemoryMB = PacketSerializer.ReadDouble(data, ref offset)
        };

        var success = _hostManager.UpdateMetrics(hostId, currentPlayers, metrics);

        var response = new List<byte> { (byte)MasterPacketType.UpdateMetricsResponse };
        PacketSerializer.WriteBool(response, success);
        PacketSerializer.WriteString(response, success ? "Metrics updated" : "Host not found");

        return PacketSerializer.CreatePacket(response);
    }

    private byte[] HandleHeartbeat(byte[] data, ref int offset)
    {
        var hostId = PacketSerializer.ReadString(data, ref offset);
        var success = _hostManager.Heartbeat(hostId);

        var response = new List<byte> { (byte)MasterPacketType.HeartbeatResponse };
        PacketSerializer.WriteBool(response, success);

        return PacketSerializer.CreatePacket(response);
    }

    private byte[] HandleGetHostList(byte[] data, ref int offset)
    {
        var hasFilters = PacketSerializer.ReadBool(data, ref offset);
        string? gameVersion = hasFilters ? PacketSerializer.ReadString(data, ref offset) : null;
        string? gameMode = hasFilters ? PacketSerializer.ReadString(data, ref offset) : null;
        bool? passwordProtected = hasFilters ? PacketSerializer.ReadBool(data, ref offset) : null;

        var hosts = _hostManager.GetActiveHosts(gameVersion, gameMode, passwordProtected);

        var response = new List<byte> { (byte)MasterPacketType.GetHostListResponse };
        PacketSerializer.WriteInt32(response, hosts.Count);

        foreach (var host in hosts)
        {
            PacketSerializer.WriteString(response, host.HostId);
            PacketSerializer.WriteString(response, host.OnlineId);
            PacketSerializer.WriteString(response, host.PublicIp);
            PacketSerializer.WriteInt32(response, host.PublicPort);
            PacketSerializer.WriteString(response, host.GameName);
            PacketSerializer.WriteString(response, host.GameMode);
            PacketSerializer.WriteString(response, host.MapName);
            PacketSerializer.WriteInt32(response, host.CurrentPlayers);
            PacketSerializer.WriteInt32(response, host.MaxPlayers);
            PacketSerializer.WriteBool(response, host.IsPasswordProtected);
            PacketSerializer.WriteString(response, host.GameVersion);
            PacketSerializer.WriteDouble(response, host.Metrics.CalculateScore());
        }

        return PacketSerializer.CreatePacket(response);
    }

    private byte[] HandleUnregisterHost(byte[] data, ref int offset)
    {
        var hostId = PacketSerializer.ReadString(data, ref offset);
        var success = _hostManager.UnregisterHost(hostId);

        var response = new List<byte> { (byte)MasterPacketType.UnregisterHostResponse };
        PacketSerializer.WriteBool(response, success);

        return PacketSerializer.CreatePacket(response);
    }

    private byte[] HandleGetStats()
    {
        var (totalHosts, activeHosts, totalSessions, totalPlayers) = _hostManager.GetStatistics();

        var response = new List<byte> { (byte)MasterPacketType.GetStatsResponse };
        PacketSerializer.WriteInt32(response, totalHosts);
        PacketSerializer.WriteInt32(response, activeHosts);
        PacketSerializer.WriteInt32(response, totalSessions);
        PacketSerializer.WriteInt32(response, totalPlayers);

        return PacketSerializer.CreatePacket(response);
    }

    private async Task<byte[]> HandleInitiateMigration(byte[] data, int startOffset)
    {
        int offset = startOffset;
        var sessionId = PacketSerializer.ReadString(data, ref offset);
        var currentHostId = PacketSerializer.ReadString(data, ref offset);
        var reason = (MigrationReason)PacketSerializer.ReadInt32(data, ref offset);
        var peerCount = PacketSerializer.ReadInt32(data, ref offset);

        var peerIds = new List<string>();
        for (int i = 0; i < peerCount; i++)
        {
            peerIds.Add(PacketSerializer.ReadString(data, ref offset));
        }

        var result = await _migrationCoordinator.InitiateMigration(sessionId, currentHostId, peerIds, reason);

        var response = new List<byte> { (byte)MasterPacketType.MigrationResponse };
        PacketSerializer.WriteBool(response, result.success);
        PacketSerializer.WriteString(response, result.newHostId ?? "");
        PacketSerializer.WriteString(response, result.newHostIp ?? "");
        PacketSerializer.WriteInt32(response, result.newHostPort);
        PacketSerializer.WriteString(response, result.message);

        return PacketSerializer.CreatePacket(response);
    }

    private byte[] HandleGetMigrationStatus(byte[] data, ref int offset)
    {
        var sessionId = PacketSerializer.ReadString(data, ref offset);
        var state = _migrationCoordinator.GetMigrationStatus(sessionId);

        var response = new List<byte> { (byte)MasterPacketType.MigrationStatusResponse };
        PacketSerializer.WriteBool(response, state != null);

        if (state != null)
        {
            PacketSerializer.WriteString(response, state.SessionId);
            PacketSerializer.WriteString(response, state.OldHostId);
            PacketSerializer.WriteString(response, state.NewHostId);
            PacketSerializer.WriteInt32(response, (int)state.Phase);
            PacketSerializer.WriteDouble(response, state.DurationMs);
        }

        return PacketSerializer.CreatePacket(response);
    }

    private byte[] HandleCancelMigration(byte[] data, ref int offset)
    {
        var sessionId = PacketSerializer.ReadString(data, ref offset);
        var success = _migrationCoordinator.CancelMigration(sessionId);

        var response = new List<byte> { (byte)MasterPacketType.CancelMigrationResponse };
        PacketSerializer.WriteBool(response, success);

        return PacketSerializer.CreatePacket(response);
    }

    private byte[] CreateErrorResponse(string message)
    {
        var response = new List<byte> { (byte)MasterPacketType.Error };
        PacketSerializer.WriteString(response, message);
        return PacketSerializer.CreatePacket(response);
    }

    // ============================================================================
    // HTTP PROTOCOL SUPPORT (for Godot client compatibility)
    // ============================================================================

    private async Task HandleHttpRequest(NetworkStream stream, byte[] firstBytes, TcpClient client)
    {
        try
        {
            // Read the full HTTP request
            var requestBuilder = new System.Text.StringBuilder();
            requestBuilder.Append(System.Text.Encoding.UTF8.GetString(firstBytes));

            var buffer = new byte[4096];
            int contentLength = 0;
            string? requestLine = null;
            var headers = new Dictionary<string, string>();

            // Read headers
            while (true)
            {
                var bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length);
                if (bytesRead == 0) break;

                requestBuilder.Append(System.Text.Encoding.UTF8.GetString(buffer, 0, bytesRead));
                var request = requestBuilder.ToString();

                // Check if we have complete headers (double newline)
                var headerEnd = request.IndexOf("\r\n\r\n");
                if (headerEnd < 0) headerEnd = request.IndexOf("\n\n");

                if (headerEnd >= 0)
                {
                    // Parse headers
                    var headerText = request.Substring(0, headerEnd);
                    var lines = headerText.Split(new[] { "\r\n", "\n" }, StringSplitOptions.RemoveEmptyEntries);

                    if (lines.Length > 0)
                    {
                        requestLine = lines[0];

                        for (int i = 1; i < lines.Length; i++)
                        {
                            var colonIndex = lines[i].IndexOf(':');
                            if (colonIndex > 0)
                            {
                                var key = lines[i].Substring(0, colonIndex).Trim();
                                var value = lines[i].Substring(colonIndex + 1).Trim();
                                headers[key.ToLowerInvariant()] = value;
                            }
                        }
                    }

                    // Check if we need to read body
                    if (headers.ContainsKey("content-length"))
                    {
                        contentLength = int.Parse(headers["content-length"]);
                        var bodyStart = headerEnd + (request[headerEnd + 2] == '\n' ? 2 : 4);
                        var bodyLength = request.Length - bodyStart;

                        // If we haven't read the full body yet, read more
                        while (bodyLength < contentLength)
                        {
                            bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length);
                            if (bytesRead == 0) break;
                            requestBuilder.Append(System.Text.Encoding.UTF8.GetString(buffer, 0, bytesRead));
                            bodyLength += bytesRead;
                        }
                    }

                    break;
                }
            }

            // Parse request
            var fullRequest = requestBuilder.ToString();
            var parts = requestLine?.Split(' ') ?? Array.Empty<string>();
            if (parts.Length < 2)
            {
                await SendHttpResponse(stream, 400, "Bad Request", "{}");
                return;
            }

            var method = parts[0];
            var path = parts[1];

            // Extract body
            var bodyStartIndex = fullRequest.IndexOf("\r\n\r\n");
            if (bodyStartIndex < 0) bodyStartIndex = fullRequest.IndexOf("\n\n");
            var body = bodyStartIndex >= 0 ? fullRequest.Substring(bodyStartIndex + 4).Trim() : "";

            // Only log important requests (not health checks or frequent monitoring)
            if (path != "/health" && path != "/api/host/update-metrics")
            {
                Console.WriteLine($"[MasterHTTP] {method} {path}");
            }

            // Route request
            var (statusCode, responseBody) = await RouteHttpRequest(method, path, body);
            await SendHttpResponse(stream, statusCode, "OK", responseBody);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[MasterHTTP] Error: {ex.Message}");
            await SendHttpResponse(stream, 500, "Internal Server Error", $"{{\"error\":\"{ex.Message}\"}}");
        }
    }

    private async Task<(int statusCode, string body)> RouteHttpRequest(string method, string path, string body)
    {
        try
        {
            // Handle different endpoints
            return (method, path) switch
            {
                ("POST", "/api/nat/register-endpoint") => await HandleHttpRegisterEndpoint(body),
                ("POST", "/api/nat/request-hole-punch") => await HandleHttpRequestHolePunch(body),
                ("POST", "/api/nat/report-success") => HandleHttpReportSuccess(body),
                ("POST", "/api/host/register") => HandleHttpRegisterHost(body),
                ("POST", "/api/host/update-metrics") => HandleHttpUpdateMetrics(body),
                ("GET", "/api/host/list") => HandleHttpGetHostList(),
                ("POST", "/api/migration/initiate") => HandleHttpInitiateMigration(body),
                ("GET", "/health") => (200, "{\"status\":\"healthy\"}"),
                _ => (404, "{\"error\":\"Not found\"}")
            };
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[MasterHTTP] Route error: {ex.Message}");
            return (500, $"{{\"error\":\"{ex.Message}\"}}");
        }
    }

    private async Task<(int, string)> HandleHttpRegisterEndpoint(string body)
    {
        // For now, just acknowledge registration
        // Full NAT traversal coordination would be implemented here
        return await Task.FromResult((200, "{\"success\":true,\"publicIp\":\"0.0.0.0\",\"publicPort\":0}"));
    }

    private async Task<(int, string)> HandleHttpRequestHolePunch(string body)
    {
        // Hole punch coordination
        return await Task.FromResult((200, "{\"success\":true,\"targetPublicIp\":\"0.0.0.0\",\"targetPublicPort\":0,\"targetPrivateIp\":\"0.0.0.0\",\"targetPrivatePort\":0,\"timestamp\":0}"));
    }

    private (int, string) HandleHttpReportSuccess(string body)
    {
        return (200, "{\"success\":true}");
    }

    private (int, string) HandleHttpRegisterHost(string body)
    {
        try
        {
            // Parse JSON (simple manual parsing to avoid dependencies)
            var hostId = ExtractJsonString(body, "hostId");
            var onlineId = ExtractJsonString(body, "onlineId");
            var privateIp = ExtractJsonString(body, "privateIp");
            var privatePort = ExtractJsonInt(body, "privatePort");
            var gameName = ExtractJsonString(body, "gameName");
            var gameMode = ExtractJsonString(body, "gameMode");
            var mapName = ExtractJsonString(body, "mapName");
            var maxPlayers = ExtractJsonInt(body, "maxPlayers");
            var isPasswordProtected = ExtractJsonBool(body, "isPasswordProtected");
            var gameVersion = ExtractJsonString(body, "gameVersion");
            var isPotentialHost = ExtractJsonBool(body, "isPotentialHost");

            var hostInfo = new HostInfo
            {
                HostId = hostId,
                OnlineId = onlineId,
                PublicIp = privateIp, // Would need actual public IP detection
                PublicPort = privatePort,
                PrivateIp = privateIp,
                PrivatePort = privatePort,
                GameName = gameName,
                GameMode = gameMode,
                MapName = mapName,
                MaxPlayers = maxPlayers,
                CurrentPlayers = 1,
                IsPasswordProtected = isPasswordProtected,
                GameVersion = gameVersion,
                IsPotentialHost = isPotentialHost
            };

            _hostManager.RegisterHost(hostInfo);

            Console.WriteLine($"[MasterHTTP] Host registered: {hostId}");

            return (200, $"{{\"success\":true,\"hostId\":\"{hostId}\"}}");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[MasterHTTP] Register host error: {ex.Message}");
            return (400, $"{{\"error\":\"{ex.Message}\"}}");
        }
    }

    private (int, string) HandleHttpUpdateMetrics(string body)
    {
        try
        {
            var hostId = ExtractJsonString(body, "hostId");
            var currentPlayers = ExtractJsonInt(body, "currentPlayers");

            // Update host metrics
            var host = _hostManager.GetHost(hostId);
            if (host != null)
            {
                host.CurrentPlayers = currentPlayers;
                host.LastHeartbeat = DateTime.UtcNow;
                //Console.WriteLine($"[MasterHTTP] Updated metrics for host: {hostId}");
            }

            return (200, "{\"success\":true}");
        }
        catch (Exception ex)
        {
            return (400, $"{{\"error\":\"{ex.Message}\"}}");
        }
    }

    private (int, string) HandleHttpGetHostList()
    {
        var hosts = _hostManager.GetActiveHosts();

        // Filter out potential hosts (clients registered for migration only)
        var actualLobbies = hosts.Where(h => !h.IsPotentialHost).ToList();

        var hostsJson = string.Join(",", actualLobbies.Select(h =>
            $"{{\"hostId\":\"{h.HostId}\",\"gameName\":\"{h.GameName}\",\"gameMode\":\"{h.GameMode}\",\"mapName\":\"{h.MapName}\",\"currentPlayers\":{h.CurrentPlayers},\"maxPlayers\":{h.MaxPlayers},\"isPasswordProtected\":{h.IsPasswordProtected.ToString().ToLower()}}}"
        ));

        return (200, $"{{\"hosts\":[{hostsJson}]}}");
    }

    private async Task SendHttpResponse(NetworkStream stream, int statusCode, string statusText, string body)
    {
        var response = $"HTTP/1.1 {statusCode} {statusText}\r\n" +
                       $"Content-Type: application/json\r\n" +
                       $"Content-Length: {body.Length}\r\n" +
                       "Access-Control-Allow-Origin: *\r\n" +
                       "Connection: close\r\n" +
                       "\r\n" +
                       body;

        var bytes = System.Text.Encoding.UTF8.GetBytes(response);
        await stream.WriteAsync(bytes, 0, bytes.Length);
    }

    // Simple JSON extraction helpers (to avoid external dependencies)
    private (int, string) HandleHttpInitiateMigration(string body)
    {
        try
        {
            // Parse migration request
            var currentHostId = ExtractJsonString(body, "currentHostId");
            var sessionId = ExtractJsonString(body, "sessionId");
            // var peerIds = ExtractJsonArray(body, "peerIds"); // Would need array parser
            var reason = ExtractJsonInt(body, "reason");

            Console.WriteLine($"[MasterHTTP] Migration request for session {sessionId}, old host: {currentHostId}, reason: {reason}");

            // Get all active hosts and select the best one based on metrics
            // CRITICAL: Immediately mark the leaving host as inactive to prevent it from being selected
            var leavingHost = _hostManager.GetHost(currentHostId);
            if (leavingHost != null)
            {
                leavingHost.IsActive = false;
                Console.WriteLine($"[MasterHTTP] Marked leaving host as inactive: {currentHostId}");
            }

            // Get ALL active hosts (including potential hosts) - don't filter by player count for migration
            var allHosts = _hostManager.GetActiveHosts();

            Console.WriteLine($"[MasterHTTP] Found {allHosts.Count} active hosts for migration");
            foreach (var h in allHosts)
            {
                Console.WriteLine($"[MasterHTTP]   - Candidate: {h.HostId} (score: {h.Metrics.CalculateScore():F2}, players: {h.CurrentPlayers}/{h.MaxPlayers})");
            }

            // Exclude the old host (double-check even though we marked it inactive)
            var candidates = allHosts.Where(h => h.HostId != currentHostId).ToList();

            Console.WriteLine($"[MasterHTTP] After excluding old host '{currentHostId}': {candidates.Count} candidates remain");

            if (candidates.Count == 0)
            {
                Console.WriteLine($"[MasterHTTP] ERROR: No suitable hosts available for migration!");
                return (200, "{\"success\":false,\"message\":\"No suitable hosts available for migration\"}");
            }

            // Select host with best metrics (highest score)
            var newHost = candidates
                .OrderByDescending(h => h.Metrics.CalculateScore())
                .First();

            Console.WriteLine($"[MasterHTTP] Selected new host: {newHost.HostId} (score: {newHost.Metrics.CalculateScore():F2})");

            // Return migration response
            var response = "{" +
                "\"success\":true," +
                $"\"newHostId\":\"{newHost.HostId}\"," +
                $"\"newHostIp\":\"{newHost.PublicIp}\"," +
                $"\"newHostPort\":{newHost.PublicPort}," +
                "\"message\":\"New host selected successfully\"" +
                "}";

            return (200, response);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[MasterHTTP] Migration error: {ex.Message}");
            Console.WriteLine($"[MasterHTTP] Stack trace: {ex.StackTrace}");
            return (500, $"{{\"success\":false,\"error\":\"{ex.Message}\"}}");
        }
    }

    private string ExtractJsonString(string json, string key)
    {
        var pattern = $"\"{key}\"\\s*:\\s*\"([^\"]*)\"";
        var match = System.Text.RegularExpressions.Regex.Match(json, pattern);
        return match.Success ? match.Groups[1].Value : "";
    }

    private int ExtractJsonInt(string json, string key)
    {
        var pattern = $"\"{key}\"\\s*:\\s*(\\d+)";
        var match = System.Text.RegularExpressions.Regex.Match(json, pattern);
        return match.Success ? int.Parse(match.Groups[1].Value) : 0;
    }

    private bool ExtractJsonBool(string json, string key)
    {
        var pattern = $"\"{key}\"\\s*:\\s*(true|false)";
        var match = System.Text.RegularExpressions.Regex.Match(json, pattern);
        return match.Success && match.Groups[1].Value == "true";
    }
}
