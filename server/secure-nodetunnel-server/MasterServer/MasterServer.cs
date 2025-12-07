using MasterServer.Core;
using MasterServer.Handlers;

namespace MasterServer;

/// <summary>
/// Master Server for NodeTunnel P2P coordination
/// Socket-based implementation (no ASP.NET)
/// </summary>
public class MasterServer
{
    public static async Task Main(string[] args)
    {
        Console.WriteLine("NodeTunnel Master Server");

        // Parse command line arguments
        int tcpPort = 9050;
        int udpPort = 9051;

        if (args.Length >= 1 && int.TryParse(args[0], out int customTcpPort))
        {
            tcpPort = customTcpPort;
        }

        if (args.Length >= 2 && int.TryParse(args[1], out int customUdpPort))
        {
            udpPort = customUdpPort;
        }

        // Initialize services
        var hostManager = new HostManagerSocket();
        var natService = new NATTraversalServiceSocket();
        var migrationCoordinator = new MigrationCoordinatorSocket(hostManager, natService);

        // Initialize handlers
        var tcpHandler = new MasterTCPHandler(hostManager, migrationCoordinator, tcpPort);
        var udpHandler = new MasterUDPHandler(natService, udpPort);

        try
        {
            // Start both handlers
            var tcpTask = tcpHandler.StartAsync();
            var udpTask = udpHandler.StartAsync();

            Console.WriteLine($"Host Registration    : TCP {tcpPort}");
            Console.WriteLine($"NAT Traversal        : UDP {udpPort}");
            Console.WriteLine($"Host Migration       : TCP {tcpPort}");
            Console.WriteLine("Press Ctrl+C to stop");
            Console.WriteLine();

            // Wait for tasks
            await Task.WhenAll(tcpTask, udpTask);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[MasterServer] Fatal error: {ex.Message}");
            Console.WriteLine(ex.StackTrace);
        }

        Console.WriteLine();
        Console.WriteLine("Master Server stopped.");
    }
}
