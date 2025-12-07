#!/bin/bash

# Run script for Fallback Relay Server

cd "$(dirname "$0")/RelayServer" || exit 1

echo "Starting NodeTunnel Fallback Relay Server..."
echo ""

# Clean any conflicting build artifacts
if [ -d "obj" ] || [ -d "bin" ]; then
    echo "Cleaning build artifacts..."
    rm -rf obj bin
fi

# Run the relay server
echo "Running from RelayServer directory..."
dotnet run
