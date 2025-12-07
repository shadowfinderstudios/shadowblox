-- Tag Game - Main Game Initialization
-- This script sets up the game world and core systems for the infection tag game

local Players = game:GetService("Players")
local RunService = game:GetService("RunService")
local ReplicatedStorage = game:GetService("ReplicatedStorage")

print("[Tag] Initializing game...")

-- Create workspace structure
local function setupWorkspace()
	local workspace = game.Workspace

	-- Create Lobby area
	local lobby = Instance.new("Model")
	lobby.Name = "Lobby"
	lobby.Parent = workspace

	-- Create LobbySpawn
	local lobbySpawn = Instance.new("SpawnLocation")
	lobbySpawn.Name = "LobbySpawn"
	lobbySpawn.Size = Vector3.new(10, 1, 10)
	lobbySpawn.Position = Vector3.new(-15, 0.5, 0)
	lobbySpawn.Anchored = true
	lobbySpawn.Neutral = true
	lobbySpawn.Enabled = true
	lobbySpawn.Parent = workspace

	-- Create Arena area
	local arena = Instance.new("Model")
	arena.Name = "Arena"
	arena.Parent = workspace

	-- Create ArenaSpawn
	local arenaSpawn = Instance.new("SpawnLocation")
	arenaSpawn.Name = "ArenaSpawn"
	arenaSpawn.Size = Vector3.new(10, 1, 10)
	arenaSpawn.Position = Vector3.new(15, 0.5, 0)
	arenaSpawn.Anchored = true
	arenaSpawn.Neutral = true
	arenaSpawn.Enabled = false
	arenaSpawn.Parent = workspace

	print("[Tag] Workspace setup complete")
end

-- Create ReplicatedStorage structure
local function setupReplicatedStorage()
	-- Create GameStatus StringValue
	local gameStatus = Instance.new("StringValue")
	gameStatus.Name = "GameStatus"
	gameStatus.Value = "Waiting for players..."
	gameStatus.Parent = ReplicatedStorage

	-- Create RemoteEvents folder
	local remoteEvents = Instance.new("Model")
	remoteEvents.Name = "RemoteEvents"
	remoteEvents.Parent = ReplicatedStorage

	-- Create RemoteFunctions folder
	local remoteFunctions = Instance.new("Model")
	remoteFunctions.Name = "RemoteFunctions"
	remoteFunctions.Parent = ReplicatedStorage

	-- Create infection event
	local infectionEvent = Instance.new("RemoteEvent")
	infectionEvent.Name = "InfectionEvent"
	infectionEvent.Parent = remoteEvents

	-- Create game state event
	local gameStateEvent = Instance.new("RemoteEvent")
	gameStateEvent.Name = "GameStateEvent"
	gameStateEvent.Parent = remoteEvents

	print("[Tag] ReplicatedStorage setup complete")
end

-- Initialize the game world
setupWorkspace()
setupReplicatedStorage()

print("[Tag] Game initialization complete!")
print("[Tag] Waiting for GameManager script to start the game loop...")
