-- GameManager (ServerScriptService)
-- This is the brain of the Tag game. It manages rounds, infection, and win conditions.

local Players = game:GetService("Players")
local RunService = game:GetService("RunService")
local ReplicatedStorage = game:GetService("ReplicatedStorage")

-- Settings
local MIN_PLAYERS = 2
local INTERMISSION_TIME = 15
local ROUND_TIME = 120

-- Game state
local gameStatus = ReplicatedStorage:WaitForChild("GameStatus")
local playersInRound = {}
local infectedPlayers = {}
local roundActive = false

-- Spawn locations
local lobbySpawn = game.Workspace:WaitForChild("LobbySpawn")
local arenaSpawn = game.Workspace:WaitForChild("ArenaSpawn")

-- Remote events
local remoteEvents = ReplicatedStorage:WaitForChild("RemoteEvents")
local infectionEvent = remoteEvents:WaitForChild("InfectionEvent")
local gameStateEvent = remoteEvents:WaitForChild("GameStateEvent")

----------------------------
-- HELPER FUNCTIONS
----------------------------

local function setStatus(text)
	gameStatus.Value = text
	print("[Game] " .. text)
	-- Broadcast to all clients
	gameStateEvent:FireAllClients(text)
end

local function getAlivePlayers()
	local alive = {}
	for _, player in pairs(playersInRound) do
		if player and player.Character and player.Character:FindFirstChild("Humanoid") then
			if player.Character.Humanoid.Health > 0 then
				table.insert(alive, player)
			end
		end
	end
	return alive
end

local function getSurvivors()
	local survivors = {}
	for _, player in pairs(getAlivePlayers()) do
		local isInfected = false
		for _, infected in pairs(infectedPlayers) do
			if infected == player then
				isInfected = true
				break
			end
		end
		if not isInfected then
			table.insert(survivors, player)
		end
	end
	return survivors
end

local function teleportPlayer(player, spawnLocation)
	if player.Character and player.Character:FindFirstChild("HumanoidRootPart") then
		local newPosition = spawnLocation.Position + Vector3.new(0, 5, 0)
		player.Character.HumanoidRootPart.Position = newPosition
	end
end

local function resetPlayer(player)
	-- In a full implementation, this would respawn the character
	-- For now, just teleport to lobby
	teleportPlayer(player, lobbySpawn)
end

----------------------------
-- INFECTION MECHANICS
----------------------------

local function makeInfected(player)
	-- Check if already infected
	for _, infected in pairs(infectedPlayers) do
		if infected == player then
			return
		end
	end

	table.insert(infectedPlayers, player)

	local character = player.Character
	if character then
		-- Visual: change appearance (in a real game, would change BrickColor)
		-- Speed boost for infected
		local humanoid = character:FindFirstChild("Humanoid")
		if humanoid then
			humanoid.WalkSpeed = 20  -- faster than normal (16)
		end
	end

	print(player.Name .. " is now INFECTED!")

	-- Notify all clients about the infection
	infectionEvent:FireAllClients(player)
end

local function setupTagging(player)
	local character = player.Character
	if not character then return end

	-- Detect when infected touches a survivor
	for _, part in pairs(character:GetChildren()) do
		if part:IsA("Part") then
			part.Touched:Connect(function(hit)
				if not roundActive then return end

				-- Check if this player is infected
				local isInfected = false
				for _, infected in pairs(infectedPlayers) do
					if infected == player then
						isInfected = true
						break
					end
				end
				if not isInfected then return end

				-- Check if hit part belongs to another player
				local hitPlayer = Players:GetPlayerFromCharacter(hit.Parent)
				if hitPlayer then
					-- Check if in round
					local inRound = false
					for _, p in pairs(playersInRound) do
						if p == hitPlayer then
							inRound = true
							break
						end
					end
					if not inRound then return end

					-- Check if not already infected
					local alreadyInfected = false
					for _, infected in pairs(infectedPlayers) do
						if infected == hitPlayer then
							alreadyInfected = true
							break
						end
					end

					if not alreadyInfected then
						makeInfected(hitPlayer)
					end
				end
			end)
		end
	end
end

----------------------------
-- ROUND MANAGEMENT
----------------------------

local function startRound()
	roundActive = true
	playersInRound = Players:GetPlayers()
	infectedPlayers = {}

	-- Teleport everyone to arena
	arenaSpawn.Enabled = true
	lobbySpawn.Enabled = false

	for _, player in pairs(playersInRound) do
		teleportPlayer(player, arenaSpawn)
		setupTagging(player)
	end

	-- Countdown before infection starts
	for i = 5, 1, -1 do
		setStatus("Infection starts in " .. i .. "...")
		task.wait(1)
	end

	-- Pick random infected
	if #playersInRound > 0 then
		local firstInfected = playersInRound[math.random(1, #playersInRound)]
		makeInfected(firstInfected)
		setStatus(firstInfected.Name .. " is INFECTED! RUN!")
	end

	-- Round timer
	local timeLeft = ROUND_TIME
	while timeLeft > 0 and roundActive do
		local survivors = getSurvivors()

		if #survivors == 0 then
			setStatus("INFECTED WIN! Everyone is infected!")
			roundActive = false
			break
		end

		if #survivors == 1 and #playersInRound > 1 then
			setStatus(survivors[1].Name .. " WINS! Last survivor!")
			roundActive = false
			break
		end

		setStatus("Survivors: " .. #survivors .. " | Time: " .. timeLeft)
		task.wait(1)
		timeLeft = timeLeft - 1
	end

	-- Round ended by timer
	if roundActive then
		local survivors = getSurvivors()
		if #survivors > 0 then
			local winnerNames = {}
			for _, p in pairs(survivors) do
				table.insert(winnerNames, p.Name)
			end
			setStatus("SURVIVORS WIN! " .. table.concat(winnerNames, ", "))
		end
	end

	roundActive = false
end

local function endRound()
	roundActive = false

	-- Send everyone back to lobby
	arenaSpawn.Enabled = false
	lobbySpawn.Enabled = true

	task.wait(3)

	for _, player in pairs(Players:GetPlayers()) do
		resetPlayer(player)
	end

	playersInRound = {}
	infectedPlayers = {}
end

----------------------------
-- MAIN GAME LOOP
----------------------------

local function gameLoop()
	while true do
		-- Intermission / waiting for players
		repeat
			local playerCount = #Players:GetPlayers()
			setStatus("Waiting for players... (" .. playerCount .. "/" .. MIN_PLAYERS .. ")")
			task.wait(1)
		until #Players:GetPlayers() >= MIN_PLAYERS

		-- Countdown to start
		local enoughPlayers = true
		for i = INTERMISSION_TIME, 1, -1 do
			setStatus("Next round in " .. i .. " seconds...")

			if #Players:GetPlayers() < MIN_PLAYERS then
				setStatus("Not enough players!")
				enoughPlayers = false
				break
			end

			task.wait(1)
		end

		-- Start round if we still have enough players
		if enoughPlayers and #Players:GetPlayers() >= MIN_PLAYERS then
			startRound()
			task.wait(5)  -- show winner message
			endRound()
		end

		task.wait(3)
	end
end

----------------------------
-- PLAYER CONNECTIONS
----------------------------

Players.PlayerAdded:Connect(function(player)
	print("[Game] Player joined: " .. player.Name)

	player.CharacterAdded:Connect(function(character)
		-- Reset appearance when character spawns
		task.wait(0.1)
		local humanoid = character:FindFirstChild("Humanoid")
		if humanoid then
			humanoid.WalkSpeed = 16  -- normal speed
		end
	end)
end)

Players.PlayerRemoving:Connect(function(player)
	print("[Game] Player leaving: " .. player.Name)

	-- Remove from tables if they leave mid-game
	for i, p in pairs(playersInRound) do
		if p == player then
			table.remove(playersInRound, i)
			break
		end
	end

	for i, p in pairs(infectedPlayers) do
		if p == player then
			table.remove(infectedPlayers, i)
			break
		end
	end
end)

-- Start the game!
print("[Game] Starting game loop...")

-- Only run game loop on server
if RunService:IsServer() then
	task.spawn(gameLoop)
end
