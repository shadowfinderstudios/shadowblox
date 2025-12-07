#!/bin/bash

# Run script for Master Server

cd "$(dirname "$0")/MasterServer" || exit 1

echo "Starting NodeTunnel Master Server..."
echo "Press Ctrl+C to stop"
echo ""

dotnet run
