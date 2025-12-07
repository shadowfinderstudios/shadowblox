-- Tag Game - Main Game Initialization
-- This script sets up the game world and core systems for the infection tag game
-- Now with lobby system support for host migration

local Players = game:GetService("Players")
local RunService = game:GetService("RunService")
local ReplicatedStorage = game:GetService("ReplicatedStorage")

print("[Tag] Initializing game with lobby support...")

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

	-- Create LobbyState folder for lobby system
	local lobbyState = Instance.new("Model")
	lobbyState.Name = "LobbyState"
	lobbyState.Parent = ReplicatedStorage

	-- Lobby name
	local lobbyName = Instance.new("StringValue")
	lobbyName.Name = "LobbyName"
	lobbyName.Value = ""
	lobbyName.Parent = lobbyState

	-- Host online ID
	local hostId = Instance.new("StringValue")
	hostId.Name = "HostOnlineId"
	hostId.Value = ""
	hostId.Parent = lobbyState

	-- Migration in progress flag
	local migrating = Instance.new("BoolValue")
	migrating.Name = "MigrationInProgress"
	migrating.Value = false
	migrating.Parent = lobbyState

	-- Player count
	local playerCount = Instance.new("IntValue")
	playerCount.Name = "PlayerCount"
	playerCount.Value = 0
	playerCount.Parent = lobbyState

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

	-- Create lobby events for host migration
	local hostMigrationStarted = Instance.new("RemoteEvent")
	hostMigrationStarted.Name = "HostMigrationStarted"
	hostMigrationStarted.Parent = remoteEvents

	local hostMigrationCompleted = Instance.new("RemoteEvent")
	hostMigrationCompleted.Name = "HostMigrationCompleted"
	hostMigrationCompleted.Parent = remoteEvents

	local hostMigrationFailed = Instance.new("RemoteEvent")
	hostMigrationFailed.Name = "HostMigrationFailed"
	hostMigrationFailed.Parent = remoteEvents

	-- World state sync event (for new players joining)
	local worldStateSync = Instance.new("RemoteEvent")
	worldStateSync.Name = "WorldStateSync"
	worldStateSync.Parent = remoteEvents

	-- Player registry sync event
	local playerRegistrySync = Instance.new("RemoteEvent")
	playerRegistrySync.Name = "PlayerRegistrySync"
	playerRegistrySync.Parent = remoteEvents

	print("[Tag] ReplicatedStorage setup complete with lobby support")
end

-- Initialize the game world
setupWorkspace()
setupReplicatedStorage()

print("[Tag] Game initialization complete!")
print("[Tag] Waiting for GameManager script to start the game loop...")
