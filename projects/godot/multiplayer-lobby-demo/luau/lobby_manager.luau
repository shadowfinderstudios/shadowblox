-- Lobby Manager - Core lobby and host migration logic in Luau
-- Handles: lobby state, player registry, host migration, world state sync
-- GDScript only handles: network transport, rendering
-- This is the authoritative source for game state

local Players = game:GetService("Players")
local RunService = game:GetService("RunService")
local ReplicatedStorage = game:GetService("ReplicatedStorage")

print("[LobbyManager] Loading lobby manager...")

-- Lobby configuration
local CONFIG = {
	MAX_PLAYERS = 16,
	HEARTBEAT_INTERVAL = 1.0,
	HOST_TIMEOUT = 5.0,
}

-- Lobby state (authoritative source for all network state)
local LobbyState = {
	-- Connection state
	isConnected = false,
	isHosting = false,
	lobbyId = "",
	lobbyName = "",
	hostOnlineId = "",

	-- Player registry: onlineId -> { userId, displayName, position, isHost, ... }
	playerRegistry = {},

	-- Our identity
	localOnlineId = "",
	localUserId = 0,
	localDisplayName = "",

	-- Host migration state
	migrationInProgress = false,
	migrationReason = "",
	playerStateBackup = {},  -- Backup before migration

	-- Heartbeat tracking for host detection
	lastHostHeartbeat = 0,
	heartbeatTimer = 0,
}

-- Helper functions
local function isServer()
	return RunService:IsServer()
end

local function isClient()
	return RunService:IsClient()
end

-- Get current time (seconds since start)
local function getTime()
	return tick()
end

-- Debug logging
local function debugLog(...)
	print("[LobbyManager]", ...)
end

-- ============================================================================
-- PLAYER REGISTRY MANAGEMENT
-- ============================================================================

-- Register a player in the lobby
local function registerPlayer(onlineId, userId, displayName, isHost)
	LobbyState.playerRegistry[onlineId] = {
		userId = userId,
		displayName = displayName,
		isHost = isHost or false,
		position = Vector3.new(0, 1, 0),
		joinTime = getTime(),
		lastSeen = getTime(),
	}

	debugLog("Registered player:", displayName, "OID:", onlineId, "UID:", userId, "Host:", isHost)

	-- Create player in Roblox data model
	local player = Players:FindFirstChild(tostring(userId))
	if not player then
		-- Player will be created by GDScript calling sbx_runtime.create_player()
		debugLog("Player", userId, "not yet in data model, waiting for GDScript")
	end

	return LobbyState.playerRegistry[onlineId]
end

-- Unregister a player from the lobby
local function unregisterPlayer(onlineId)
	local playerData = LobbyState.playerRegistry[onlineId]
	if playerData then
		debugLog("Unregistering player:", playerData.displayName, "OID:", onlineId)
		LobbyState.playerRegistry[onlineId] = nil
	end
end

-- Update player position in registry
local function updatePlayerPosition(onlineId, position)
	local playerData = LobbyState.playerRegistry[onlineId]
	if playerData then
		playerData.position = position
		playerData.lastSeen = getTime()
	end
end

-- Get player by user ID
local function getPlayerByUserId(userId)
	for onlineId, data in pairs(LobbyState.playerRegistry) do
		if data.userId == userId then
			return onlineId, data
		end
	end
	return nil, nil
end

-- Get all players as array for sync
local function getAllPlayersArray()
	local players = {}
	for onlineId, data in pairs(LobbyState.playerRegistry) do
		table.insert(players, {
			onlineId = onlineId,
			userId = data.userId,
			displayName = data.displayName,
			isHost = data.isHost,
			position = {x = data.position.X, y = data.position.Y, z = data.position.Z},
		})
	end
	return players
end

-- Get player count
local function getPlayerCount()
	local count = 0
	for _ in pairs(LobbyState.playerRegistry) do
		count = count + 1
	end
	return count
end

-- ============================================================================
-- LOBBY LIFECYCLE
-- ============================================================================

-- Initialize as host
local function initializeAsHost(lobbyName, onlineId, displayName)
	LobbyState.isHosting = true
	LobbyState.isConnected = true
	LobbyState.lobbyName = lobbyName
	LobbyState.lobbyId = onlineId
	LobbyState.hostOnlineId = onlineId
	LobbyState.localOnlineId = onlineId
	LobbyState.localDisplayName = displayName
	LobbyState.localUserId = 1  -- Host is always user ID 1

	-- Register ourselves as host
	registerPlayer(onlineId, 1, displayName, true)

	debugLog("Initialized as host. Lobby:", lobbyName, "ID:", onlineId)
end

-- Initialize as client joining a lobby
local function initializeAsClient(hostId, onlineId, displayName)
	LobbyState.isHosting = false
	LobbyState.isConnected = true
	LobbyState.hostOnlineId = hostId
	LobbyState.localOnlineId = onlineId
	LobbyState.localDisplayName = displayName
	-- localUserId will be assigned by the host

	debugLog("Initialized as client. Host:", hostId, "Our OID:", onlineId)
end

-- Clean up lobby state when leaving
local function cleanupLobby()
	debugLog("Cleaning up lobby state")

	LobbyState.isConnected = false
	LobbyState.isHosting = false
	LobbyState.lobbyId = ""
	LobbyState.lobbyName = ""
	LobbyState.hostOnlineId = ""
	LobbyState.playerRegistry = {}
	LobbyState.migrationInProgress = false
	LobbyState.playerStateBackup = {}
end

-- ============================================================================
-- HOST MIGRATION
-- ============================================================================

-- Start host migration process
local function startHostMigration(reason)
	if LobbyState.migrationInProgress then
		debugLog("Migration already in progress, ignoring")
		return
	end

	debugLog("Starting host migration. Reason:", reason)

	LobbyState.migrationInProgress = true
	LobbyState.migrationReason = reason

	-- Backup current player states
	LobbyState.playerStateBackup = {}
	for onlineId, data in pairs(LobbyState.playerRegistry) do
		LobbyState.playerStateBackup[onlineId] = {
			userId = data.userId,
			displayName = data.displayName,
			isHost = data.isHost,
			position = {x = data.position.X, y = data.position.Y, z = data.position.Z},
		}
	end

	debugLog("Backed up", #LobbyState.playerStateBackup, "player states")

	-- Signal to GDScript that migration started
	if onHostMigrationStarted then
		onHostMigrationStarted(reason)
	end
end

-- Complete host migration - we became the new host
local function becomeNewHost(newUserId)
	debugLog("Becoming new host with userId:", newUserId)

	LobbyState.isHosting = true
	LobbyState.hostOnlineId = LobbyState.localOnlineId

	-- Update our own entry in the registry
	local ourData = LobbyState.playerRegistry[LobbyState.localOnlineId]
	if ourData then
		local oldUserId = ourData.userId
		ourData.userId = newUserId
		ourData.isHost = true
		debugLog("Updated our userId from", oldUserId, "to", newUserId)
	end

	-- Mark old host as gone
	for onlineId, data in pairs(LobbyState.playerRegistry) do
		if data.isHost and onlineId ~= LobbyState.localOnlineId then
			debugLog("Removing old host:", onlineId)
			LobbyState.playerRegistry[onlineId] = nil
		end
	end

	LobbyState.migrationInProgress = false
	debugLog("Host migration completed - we are now host")

	-- Signal to GDScript
	if onHostMigrationCompleted then
		onHostMigrationCompleted(LobbyState.localOnlineId)
	end
end

-- Handle migration completed (we're not the new host)
local function handleMigrationCompleted(newHostOnlineId)
	debugLog("Host migration completed. New host:", newHostOnlineId)

	LobbyState.hostOnlineId = newHostOnlineId
	LobbyState.migrationInProgress = false

	-- Update host flag in registry
	for onlineId, data in pairs(LobbyState.playerRegistry) do
		data.isHost = (onlineId == newHostOnlineId)
	end

	-- Signal to GDScript
	if onHostMigrationCompleted then
		onHostMigrationCompleted(newHostOnlineId)
	end
end

-- Handle migration failure
local function handleMigrationFailed(reason)
	debugLog("Host migration failed:", reason)

	LobbyState.migrationInProgress = false
	LobbyState.playerStateBackup = {}

	-- Signal to GDScript
	if onHostMigrationFailed then
		onHostMigrationFailed(reason)
	end
end

-- Restore player states from backup (after migration)
local function restorePlayerStates()
	if not LobbyState.playerStateBackup then
		debugLog("No player state backup to restore")
		return
	end

	debugLog("Restoring player states from backup")

	for onlineId, backup in pairs(LobbyState.playerStateBackup) do
		local existing = LobbyState.playerRegistry[onlineId]
		if existing then
			-- Restore position
			existing.position = Vector3.new(backup.position.x, backup.position.y, backup.position.z)
			debugLog("Restored position for", onlineId)
		end
	end

	LobbyState.playerStateBackup = {}
end

-- ============================================================================
-- WORLD STATE SYNC
-- ============================================================================

-- Build world state dictionary for sync to new players
local function buildWorldState()
	local state = {
		players = getAllPlayersArray(),
		lobbyName = LobbyState.lobbyName,
		hostOnlineId = LobbyState.hostOnlineId,
		-- Game-specific state will be added by game_manager.luau
	}
	return state
end

-- Apply world state received from host
local function applyWorldState(state)
	debugLog("Applying world state from host")

	if state.players then
		for _, playerData in ipairs(state.players) do
			local existing = LobbyState.playerRegistry[playerData.onlineId]
			if existing then
				existing.position = Vector3.new(
					playerData.position.x,
					playerData.position.y,
					playerData.position.z
				)
			else
				registerPlayer(
					playerData.onlineId,
					playerData.userId,
					playerData.displayName,
					playerData.isHost
				)
				local newPlayer = LobbyState.playerRegistry[playerData.onlineId]
				if newPlayer then
					newPlayer.position = Vector3.new(
						playerData.position.x,
						playerData.position.y,
						playerData.position.z
					)
				end
			end
		end
	end

	if state.hostOnlineId then
		LobbyState.hostOnlineId = state.hostOnlineId
	end
end

-- ============================================================================
-- NETWORK EVENT HANDLERS (Called from GDScript)
-- ============================================================================

-- Called when a peer connects (server-side)
local function onPeerConnected(peerId, onlineId, displayName)
	debugLog("Peer connected. PeerID:", peerId, "OID:", onlineId, "Name:", displayName)

	if isServer() then
		-- Assign a user ID and register the player
		local userId = peerId  -- Use peer ID as user ID
		registerPlayer(onlineId, userId, displayName, false)

		return {
			userId = userId,
			worldState = buildWorldState(),
		}
	end
end

-- Called when a peer disconnects
local function onPeerDisconnected(peerId, onlineId)
	debugLog("Peer disconnected. PeerID:", peerId, "OID:", onlineId)

	-- Skip removal during migration
	if LobbyState.migrationInProgress then
		debugLog("Skipping removal during migration")
		return
	end

	local playerData = LobbyState.playerRegistry[onlineId]
	if playerData then
		-- Check if the host left
		if playerData.isHost and not LobbyState.isHosting then
			debugLog("Host disconnected! Starting migration...")
			startHostMigration("HOST_LEFT")
		end

		unregisterPlayer(onlineId)
	end
end

-- Called when we successfully join a lobby
local function onJoinedLobby(hostOnlineId, ourOnlineId, assignedUserId, displayName)
	debugLog("Joined lobby. Host:", hostOnlineId, "Our OID:", ourOnlineId, "UID:", assignedUserId)

	LobbyState.localUserId = assignedUserId
	initializeAsClient(hostOnlineId, ourOnlineId, displayName)
	registerPlayer(ourOnlineId, assignedUserId, displayName, false)
end

-- Called when we start hosting
local function onHostingStarted(lobbyName, onlineId, displayName)
	debugLog("Started hosting. Lobby:", lobbyName, "OID:", onlineId)
	initializeAsHost(lobbyName, onlineId, displayName)
end

-- Called when we leave the room
local function onLeftRoom()
	debugLog("Left room")
	cleanupLobby()
end

-- ============================================================================
-- HEARTBEAT / MAIN LOOP
-- ============================================================================

-- Update heartbeat and check for host timeout
RunService.Heartbeat:Connect(function(deltaTime)
	if not LobbyState.isConnected then return end

	-- Host heartbeat check (client-side)
	if not LobbyState.isHosting and not LobbyState.migrationInProgress then
		LobbyState.heartbeatTimer = LobbyState.heartbeatTimer + deltaTime

		-- Check if host might be gone (this is backup - NodeTunnel handles this)
		-- We rely on NodeTunnel's host migration, this is just for awareness
	end

	-- Update player positions from Roblox data model
	for _, player in ipairs(Players:GetPlayers()) do
		if player and player.Character then
			local rootPart = player.Character:FindFirstChild("HumanoidRootPart")
			if rootPart then
				local onlineId, _ = getPlayerByUserId(player.UserId)
				if onlineId then
					updatePlayerPosition(onlineId, rootPart.Position)
				end
			end
		end
	end
end)

-- ============================================================================
-- EXPORTS
-- ============================================================================

print("[LobbyManager] Lobby manager initialized!")

-- Return API for GDScript to call
return {
	-- State getters
	getState = function() return LobbyState end,
	isHosting = function() return LobbyState.isHosting end,
	isConnected = function() return LobbyState.isConnected end,
	isMigrating = function() return LobbyState.migrationInProgress end,
	getLocalUserId = function() return LobbyState.localUserId end,
	getLocalOnlineId = function() return LobbyState.localOnlineId end,
	getHostOnlineId = function() return LobbyState.hostOnlineId end,
	getPlayerCount = getPlayerCount,
	getAllPlayers = getAllPlayersArray,

	-- Player management
	registerPlayer = registerPlayer,
	unregisterPlayer = unregisterPlayer,
	updatePlayerPosition = updatePlayerPosition,
	getPlayerByUserId = getPlayerByUserId,

	-- Lifecycle
	initializeAsHost = initializeAsHost,
	initializeAsClient = initializeAsClient,
	cleanupLobby = cleanupLobby,

	-- Event handlers (called from GDScript)
	onHostingStarted = onHostingStarted,
	onJoinedLobby = onJoinedLobby,
	onPeerConnected = onPeerConnected,
	onPeerDisconnected = onPeerDisconnected,
	onLeftRoom = onLeftRoom,

	-- Host migration
	startHostMigration = startHostMigration,
	becomeNewHost = becomeNewHost,
	handleMigrationCompleted = handleMigrationCompleted,
	handleMigrationFailed = handleMigrationFailed,
	restorePlayerStates = restorePlayerStates,

	-- World state sync
	buildWorldState = buildWorldState,
	applyWorldState = applyWorldState,

	-- Config
	CONFIG = CONFIG,
}
