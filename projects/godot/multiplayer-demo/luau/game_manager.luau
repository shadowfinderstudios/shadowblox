-- Tag Game Manager - Core game logic in Luau
-- Uses SbxRuntime's generic rendering API to control GDScript visuals
-- GDScript handles: rendering, input forwarding, network transport

local Players = game:GetService("Players")
local RunService = game:GetService("RunService")

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
}

-- Helper functions to check server/client status dynamically
-- These must be called each time since the status is set AFTER scripts load
local function isServer()
	return RunService:IsServer()
end

local function isClient()
	return RunService:IsClient()
end

-- Debug: will show initial state (may change after Host/Join is pressed)
print("[TagGame] Initial IsServer: " .. tostring(isServer()))
print("[TagGame] Initial IsClient: " .. tostring(isClient()))

-- Colors for players
local COLORS = {
	IT = Color3.new(1, 0, 0),        -- Red for "it"
	LOCAL = Color3.new(0, 1, 0),     -- Green for local player
	REMOTE = Color3.new(0, 0, 1),    -- Blue for other players
}

-- Update player position based on humanoid MoveDirection
local function updatePlayerMovement(player, deltaTime)
	if not player then return end

	local character = player.Character
	if not character then return end

	local rootPart = character:FindFirstChild("HumanoidRootPart")
	if not rootPart then return end

	local humanoid = character:FindFirstChild("Humanoid")
	if not humanoid then return end

	-- Get move direction (set by GDScript via set_input_direction)
	local moveDir = humanoid.MoveDirection
	if moveDir and moveDir.Magnitude > 0.01 then
		local speed = CONFIG.WALK_SPEED * deltaTime
		local currentPos = rootPart.Position
		local newPos = currentPos + (moveDir.Unit * speed)

		-- Clamp to bounds
		newPos = Vector3.new(
			math.clamp(newPos.X, CONFIG.BOUNDS.min, CONFIG.BOUNDS.max),
			currentPos.Y,
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

-- Update player colors based on tag state
local function updatePlayerColors()
	for _, player in ipairs(Players:GetPlayers()) do
		if player then
			local userId = player.UserId
			local color = COLORS.REMOTE

			if userId == GameState.taggedPlayerId then
				color = COLORS.IT
			elseif Players.LocalPlayer and userId == Players.LocalPlayer.UserId then
				color = COLORS.LOCAL
			end

			-- Call SbxRuntime to update color (emits signal to GDScript)
			-- Note: This would need to be implemented via a callback mechanism
			-- For now, we track state and GDScript reads it
		end
	end
end

-- Update status text
local function updateStatusText()
	local playerCount = #Players:GetPlayers()
	local statusText = ""

	if isServer() then
		statusText = "Server - " .. playerCount .. " players"
	else
		statusText = "Client - " .. playerCount .. " players"
	end

	if GameState.taggedPlayerId > 0 then
		if Players.LocalPlayer and GameState.taggedPlayerId == Players.LocalPlayer.UserId then
			statusText = statusText .. " - YOU'RE IT!"
		else
			statusText = statusText .. " - Player " .. GameState.taggedPlayerId .. " is IT"
		end
	end

	-- Would call set_status_text here if we had the callback
	print("[TagGame] Status: " .. statusText)
end

-- Tag a player (make them "it")
local function tagPlayer(newTaggedId)
	if GameState.taggedPlayerId == newTaggedId then return end

	local oldTagged = GameState.taggedPlayerId
	GameState.taggedPlayerId = newTaggedId

	-- Set cooldown on the newly tagged player
	GameState.tagCooldowns[newTaggedId] = CONFIG.TAG_COOLDOWN

	print("[TagGame] Player " .. tostring(newTaggedId) .. " is now IT!")

	updatePlayerColors()
	updateStatusText()
end

-- Check for tag collisions (server only)
local function checkTagCollisions()
	if not isServer() then return end
	if GameState.taggedPlayerId == 0 then return end

	local taggerId = GameState.taggedPlayerId
	if not canTag(taggerId) then return end

	local tagger = nil
	for _, player in ipairs(Players:GetPlayers()) do
		if player and player.UserId == taggerId then
			tagger = player
			break
		end
	end
	if not tagger then return end

	-- Check distance to all other players
	for _, player in ipairs(Players:GetPlayers()) do
		if player and player.UserId ~= taggerId then
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
	-- Update all player movements (with safe iteration)
	local players = Players:GetPlayers()
	for i = 1, #players do
		local player = players[i]
		if player then
			updatePlayerMovement(player, deltaTime)
		end
	end

	-- Server-only logic
	if isServer() then
		updateCooldowns(deltaTime)
		checkTagCollisions()
	end
end)

-- Handle player joining
Players.PlayerAdded:Connect(function(player)
	if not player then return end

	print("[TagGame] Player added: " .. player.Name .. " (ID: " .. tostring(player.UserId) .. ")")

	-- Initialize cooldown
	GameState.tagCooldowns[player.UserId] = 0

	-- If this is server and no one is "it", make first player "it"
	if isServer() and GameState.taggedPlayerId == 0 then
		local allPlayers = Players:GetPlayers()
		if #allPlayers >= 1 and allPlayers[1] then
			tagPlayer(allPlayers[1].UserId)
		end
	end

	-- When character loads, set initial position
	player.CharacterAdded:Connect(function(character)
		if not character then return end
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

		updatePlayerColors()
	end)
end)

-- Handle player leaving
Players.PlayerRemoving:Connect(function(player)
	if not player then return end

	print("[TagGame] Player removing: " .. player.Name)
	GameState.tagCooldowns[player.UserId] = nil

	-- If the tagged player left, assign a new one
	if isServer() and GameState.taggedPlayerId == player.UserId then
		GameState.taggedPlayerId = 0
		local remainingPlayers = Players:GetPlayers()
		for _, p in ipairs(remainingPlayers) do
			if p and p.UserId ~= player.UserId then
				tagPlayer(p.UserId)
				break
			end
		end
	end
end)

-- Handle existing players (for late-joining)
for _, player in ipairs(Players:GetPlayers()) do
	if player then
		print("[TagGame] Existing player: " .. player.Name)
		GameState.tagCooldowns[player.UserId] = 0
	end
end

print("[TagGame] Game manager initialized!")

-- Export game state for external access
return {
	getTaggedPlayerId = function()
		return GameState.taggedPlayerId
	end,
	getConfig = function()
		return CONFIG
	end,
}
