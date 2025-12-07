-- Game Initialization Script
-- Sets up the escape room world using shadowblox DataModel

-- Get services
local Players = game:GetService("Players")
local RunService = game:GetService("RunService")

-- Wait for local player
local player = Players.LocalPlayer or Players.PlayerAdded:Wait()

print("Escape Room Demo - Initializing...")

-- Create the player's character when they join
Players.PlayerAdded:Connect(function(newPlayer)
	-- Create a simple character model
	local character = Instance.new("Model")
	character.Name = newPlayer.DisplayName .. "'s Character"

	-- Create HumanoidRootPart
	local rootPart = Instance.new("Part")
	rootPart.Name = "HumanoidRootPart"
	rootPart.Size = Vector3.new(2, 2, 1)
	rootPart.Position = Vector3.new(0, 3, 0)
	rootPart.Anchored = false
	rootPart.CanCollide = true
	rootPart.Parent = character

	character.PrimaryPart = rootPart
	character.Parent = workspace

	newPlayer.Character = character

	print(newPlayer.DisplayName .. " spawned!")
end)

-- Build Room 1: Key Puzzle
local function buildRoom1()
	-- Create key
	local key = Instance.new("Part")
	key.Name = "GoldenKey"
	key.Size = Vector3.new(0.3, 0.1, 0.8)
	key.Position = Vector3.new(15, 1, -3)
	key.Anchored = true
	key.CanTouch = true
	key.Parent = workspace

	-- Attach key script
	local keyScript = Instance.new("Script")
	keyScript.Name = "KeyScript"
	keyScript.Parent = key
	-- In full implementation, script source would be loaded here

	-- Create door
	local door = Instance.new("Model")
	door.Name = "Door1"
	door:SetAttribute("RequiredKey", "GoldenKey")

	local doorPart = Instance.new("Part")
	doorPart.Name = "DoorPart"
	doorPart.Size = Vector3.new(4, 4, 0.5)
	doorPart.Position = Vector3.new(18, 2, 0)
	doorPart.Anchored = true
	doorPart.CanCollide = true
	doorPart.Parent = door

	local trigger = Instance.new("Part")
	trigger.Name = "TriggerArea"
	trigger.Size = Vector3.new(2, 4, 2)
	trigger.Position = Vector3.new(17, 2, 0)
	trigger.Transparency = 1
	trigger.CanCollide = false
	trigger.CanTouch = true
	trigger.Parent = door

	door.Parent = workspace
end

-- Build Room 2: Button Puzzle
local function buildRoom2()
	local puzzle = Instance.new("Model")
	puzzle.Name = "ButtonPuzzle"
	puzzle:SetAttribute("Door", "Door2")

	-- Red button
	local button1 = Instance.new("Part")
	button1.Name = "Button1"
	button1.Size = Vector3.new(1, 0.2, 1)
	button1.Position = Vector3.new(22, 1.2, -4)
	button1:SetAttribute("ButtonId", 1)
	button1.Anchored = true
	button1.CanTouch = true
	button1.Parent = puzzle

	-- Blue button
	local button2 = Instance.new("Part")
	button2.Name = "Button2"
	button2.Size = Vector3.new(1, 0.2, 1)
	button2.Position = Vector3.new(24, 1.2, -4)
	button2:SetAttribute("ButtonId", 2)
	button2.Anchored = true
	button2.CanTouch = true
	button2.Parent = puzzle

	-- Green button
	local button3 = Instance.new("Part")
	button3.Name = "Button3"
	button3.Size = Vector3.new(1, 0.2, 1)
	button3.Position = Vector3.new(26, 1.2, -4)
	button3:SetAttribute("ButtonId", 3)
	button3.Anchored = true
	button3.CanTouch = true
	button3.Parent = puzzle

	puzzle.Parent = workspace

	-- Create door
	local door = Instance.new("Model")
	door.Name = "Door2"

	local doorPart = Instance.new("Part")
	doorPart.Name = "DoorPart"
	doorPart.Size = Vector3.new(4, 4, 0.5)
	doorPart.Position = Vector3.new(30, 2, 0)
	doorPart.Anchored = true
	doorPart.Parent = door

	door.Parent = workspace
end

-- Build Room 3: Code Lock
local function buildRoom3()
	local codeLock = Instance.new("Model")
	codeLock.Name = "CodeLock"
	codeLock:SetAttribute("CorrectCode", "2048")
	codeLock:SetAttribute("Door", "Door3")

	local lockPart = Instance.new("Part")
	lockPart.Name = "LockPanel"
	lockPart.Size = Vector3.new(0.5, 0.5, 0.1)
	lockPart.Position = Vector3.new(36, 1.5, -5.7)
	lockPart.Anchored = true
	lockPart.Parent = codeLock

	local trigger = Instance.new("Part")
	trigger.Name = "TriggerArea"
	trigger.Size = Vector3.new(2, 2, 2)
	trigger.Position = Vector3.new(36, 1.5, -5)
	trigger.Transparency = 1
	trigger.CanCollide = false
	trigger.CanTouch = true
	trigger.Parent = codeLock

	codeLock.Parent = workspace

	-- Create door
	local door = Instance.new("Model")
	door.Name = "Door3"

	local doorPart = Instance.new("Part")
	doorPart.Name = "DoorPart"
	doorPart.Size = Vector3.new(4, 4, 0.5)
	doorPart.Position = Vector3.new(42, 2, 0)
	doorPart.Anchored = true
	doorPart.Parent = door

	door.Parent = workspace
end

-- Build Room 4: Pressure Plate
local function buildRoom4()
	local plate = Instance.new("Part")
	plate.Name = "PressurePlate"
	plate.Size = Vector3.new(1.5, 0.1, 1.5)
	plate.Position = Vector3.new(48, 0.05, 0)
	plate.Anchored = true
	plate.CanTouch = true
	plate:SetAttribute("Door", "Door4")
	plate:SetAttribute("StayOpen", true)
	plate.Parent = workspace

	-- Create door
	local door = Instance.new("Model")
	door.Name = "Door4"

	local doorPart = Instance.new("Part")
	doorPart.Name = "DoorPart"
	doorPart.Size = Vector3.new(4, 4, 0.5)
	doorPart.Position = Vector3.new(54, 2, 0)
	doorPart.Anchored = true
	doorPart.Parent = door

	door.Parent = workspace
end

-- Build Win Area
local function buildWinArea()
	local winArea = Instance.new("Part")
	winArea.Name = "WinArea"
	winArea.Size = Vector3.new(3, 3, 3)
	winArea.Position = Vector3.new(60, 1.5, 0)
	winArea.Transparency = 0.5
	winArea.CanCollide = false
	winArea.CanTouch = true
	winArea.Parent = workspace
end

-- Initialize all rooms
buildRoom1()
buildRoom2()
buildRoom3()
buildRoom4()
buildWinArea()

print("Escape Room ready! Find your way to the exit!")
print("Room 1: Find the golden key")
print("Room 2: Press buttons: Red -> Blue -> Green")
print("Room 3: Enter code 2048")
print("Room 4: Step on the pressure plate")
