-- Tag Game Manager - Core game logic in Luau
-- Manages player movement, tagging, and game state
-- GDScript handles: rendering, input forwarding, network transport

local Players = game:GetService("Players")
local RunService = game:GetService("RunService")
local ReplicatedStorage = game:GetService("ReplicatedStorage")

print("[TagGame] Loading game manager...")

-- Game configuration
local CONFIG = {
	WALK_SPEED = 5.0,
	TAG_DISTANCE = 2.0,
	TAG_COOLDOWN = 1.0,
	BOUNDS = { min = -20, max = 20 },
}

-- Game state
local GameState = {
	taggedPlayerId = 0,
	tagCooldowns = {},  -- userId -> time remaining
	isServer = RunService:IsServer(),
	isClient = RunService:IsClient(),
}

print("[TagGame] IsServer: " .. tostring(GameState.isServer))
print("[TagGame] IsClient: " .. tostring(GameState.isClient))

-- Get RemoteEvents folder
local function getRemoteEvents()
	local remoteEvents = ReplicatedStorage:FindFirstChild("RemoteEvents")
	return remoteEvents
end

-- Update player position based on humanoid MoveDirection
-- MoveDirection is set by GDScript via set_input_direction()
local function updatePlayerMovement(player, deltaTime)
	local character = player.Character
	if not character then return end

	local rootPart = character:FindFirstChild("HumanoidRootPart")
	if not rootPart then return end

	local humanoid = character:FindFirstChild("Humanoid")
	if not humanoid then return end

	-- Get move direction (set by GDScript)
	local moveDir = humanoid.MoveDirection
	if moveDir.Magnitude > 0.01 then
		local speed = CONFIG.WALK_SPEED * deltaTime
		local currentPos = rootPart.Position
		local newPos = currentPos + (moveDir.Unit * speed)

		-- Clamp to bounds
		newPos = Vector3.new(
			math.clamp(newPos.X, CONFIG.BOUNDS.min, CONFIG.BOUNDS.max),
			currentPos.Y,  -- Keep Y unchanged
			math.clamp(newPos.Z, CONFIG.BOUNDS.min, CONFIG.BOUNDS.max)
		)

		rootPart.Position = newPos
	end
end

-- Check if a player can tag (not on cooldown)
local function canTag(userId)
	local cooldown = GameState.tagCooldowns[userId] or 0
	return cooldown <= 0
end

-- Get distance between two players
local function getPlayerDistance(player1, player2)
	if not player1 or not player2 then return math.huge end

	local char1 = player1.Character
	local char2 = player2.Character
	if not char1 or not char2 then return math.huge end

	local root1 = char1:FindFirstChild("HumanoidRootPart")
	local root2 = char2:FindFirstChild("HumanoidRootPart")
	if not root1 or not root2 then return math.huge end

	return (root1.Position - root2.Position).Magnitude
end

-- Tag a player (make them "it")
local function tagPlayer(newTaggedId)
	if GameState.taggedPlayerId == newTaggedId then return end

	local oldTagged = GameState.taggedPlayerId
	GameState.taggedPlayerId = newTaggedId

	-- Set cooldown on the newly tagged player
	GameState.tagCooldowns[newTaggedId] = CONFIG.TAG_COOLDOWN

	print("[TagGame] Player " .. tostring(newTaggedId) .. " is now IT!")

	-- Broadcast to clients via RemoteEvent
	local remoteEvents = getRemoteEvents()
	if remoteEvents and GameState.isServer then
		local gameStateEvent = remoteEvents:FindFirstChild("GameStateEvent")
		if gameStateEvent then
			gameStateEvent:FireAllClients(newTaggedId)
		end
	end
end

-- Check for tag collisions (server only)
local function checkTagCollisions()
	if not GameState.isServer then return end
	if GameState.taggedPlayerId == 0 then return end

	local taggerId = GameState.taggedPlayerId
	if not canTag(taggerId) then return end

	local tagger = nil
	for _, player in ipairs(Players:GetPlayers()) do
		if player.UserId == taggerId then
			tagger = player
			break
		end
	end
	if not tagger then return end

	-- Check distance to all other players
	for _, player in ipairs(Players:GetPlayers()) do
		if player.UserId ~= taggerId then
			local distance = getPlayerDistance(tagger, player)
			if distance < CONFIG.TAG_DISTANCE then
				-- Tag them!
				tagPlayer(player.UserId)
				break
			end
		end
	end
end

-- Update cooldowns
local function updateCooldowns(deltaTime)
	for userId, cooldown in pairs(GameState.tagCooldowns) do
		if cooldown > 0 then
			GameState.tagCooldowns[userId] = cooldown - deltaTime
		end
	end
end

-- Main game loop via Heartbeat
RunService.Heartbeat:Connect(function(deltaTime)
	-- Update all player movements
	for _, player in ipairs(Players:GetPlayers()) do
		updatePlayerMovement(player, deltaTime)
	end

	-- Server-only logic
	if GameState.isServer then
		updateCooldowns(deltaTime)
		checkTagCollisions()
	end
end)

-- Handle player joining
Players.PlayerAdded:Connect(function(player)
	print("[TagGame] Player added: " .. player.Name .. " (ID: " .. tostring(player.UserId) .. ")")

	-- Initialize cooldown
	GameState.tagCooldowns[player.UserId] = 0

	-- If this is server and no one is "it", make first player "it"
	if GameState.isServer and GameState.taggedPlayerId == 0 then
		local allPlayers = Players:GetPlayers()
		if #allPlayers >= 1 then
			tagPlayer(allPlayers[1].UserId)
		end
	end

	-- When character loads, set initial position
	player.CharacterAdded:Connect(function(character)
		print("[TagGame] Character loaded for: " .. player.Name)

		local rootPart = character:FindFirstChild("HumanoidRootPart")
		if rootPart then
			-- Random spawn position
			local randomOffset = Vector3.new(
				math.random(-5, 5),
				1,
				math.random(-5, 5)
			)
			rootPart.Position = randomOffset
		end
	end)
end)

-- Handle player leaving
Players.PlayerRemoving:Connect(function(player)
	print("[TagGame] Player removing: " .. player.Name)
	GameState.tagCooldowns[player.UserId] = nil

	-- If the tagged player left, assign a new one
	if GameState.isServer and GameState.taggedPlayerId == player.UserId then
		GameState.taggedPlayerId = 0
		local remainingPlayers = Players:GetPlayers()
		for _, p in ipairs(remainingPlayers) do
			if p.UserId ~= player.UserId then
				tagPlayer(p.UserId)
				break
			end
		end
	end
end)

-- Handle existing players (for late-joining clients)
for _, player in ipairs(Players:GetPlayers()) do
	print("[TagGame] Existing player: " .. player.Name)
	GameState.tagCooldowns[player.UserId] = 0
end

-- Client: Listen for tag updates from server
if GameState.isClient then
	local remoteEvents = getRemoteEvents()
	if remoteEvents then
		local gameStateEvent = remoteEvents:FindFirstChild("GameStateEvent")
		if gameStateEvent then
			gameStateEvent.OnClientEvent:Connect(function(newTaggedId)
				GameState.taggedPlayerId = newTaggedId
				print("[TagGame] Client received: Player " .. tostring(newTaggedId) .. " is now IT!")
			end)
		end
	end
end

print("[TagGame] Game manager initialized!")

-- Export game state for external access
return {
	getTaggedPlayerId = function()
		return GameState.taggedPlayerId
	end,
	setTaggedPlayer = function(userId)
		if GameState.isServer then
			tagPlayer(userId)
		end
	end,
	getConfig = function()
		return CONFIG
	end,
}
