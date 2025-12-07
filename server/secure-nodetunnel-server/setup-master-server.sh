#!/bin/bash

# Setup script for Master Server

echo "==================================="
echo "NodeTunnel Master Server Setup"
echo "==================================="
echo ""

# Check if dotnet is installed
if ! command -v dotnet &> /dev/null; then
    echo "ERROR: .NET SDK not found!"
    echo ""
    echo "Please install .NET 9.0 SDK:"
    echo "  Ubuntu/Debian:"
    echo "    wget https://dot.net/v1/dotnet-install.sh"
    echo "    chmod +x dotnet-install.sh"
    echo "    ./dotnet-install.sh --channel 9.0"
    echo ""
    echo "  Or visit: https://dotnet.microsoft.com/download"
    exit 1
fi

echo "✓ .NET SDK found: $(dotnet --version)"
echo ""

# Navigate to MasterServer directory
cd "$(dirname "$0")/MasterServer" || exit 1

echo "Restoring NuGet packages..."
dotnet restore

if [ $? -eq 0 ]; then
    echo "✓ Packages restored successfully"
    echo ""
    echo "Building project..."
    dotnet build

    if [ $? -eq 0 ]; then
        echo "✓ Build successful"
        echo ""
        echo "==================================="
        echo "Setup complete!"
        echo "==================================="
        echo ""
        echo "To run the Master Server:"
        echo "  cd MasterServer"
        echo "  dotnet run"
        echo ""
        echo "Or use:"
        echo "  ./run-master-server.sh"
        echo ""
    else
        echo "✗ Build failed"
        exit 1
    fi
else
    echo "✗ Package restore failed"
    exit 1
fi
