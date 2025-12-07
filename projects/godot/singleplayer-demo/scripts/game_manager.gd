extends Node3D
## ShadowBlox Game Manager
## Demonstrates the Luau integration with Godot for the escape room demo

@onready var sbx_runtime: SbxRuntime = $SbxRuntime

var player_character: SbxPart = null

func _ready() -> void:
	# Wait for runtime to initialize
	await get_tree().process_frame

	# Create the local player
	sbx_runtime.create_local_player(1, "Player1")

	# Set up the game using Luau
	_setup_game_world()

func _setup_game_world() -> void:
	# Run the key collection script on the key
	var key_part = $Room1_Key/GoldenKey as SbxPart
	if key_part:
		var key_script = """
local key = script.Parent
print("Key script loaded for: " .. key.Name)

key.Touched:Connect(function(otherPart)
	print("Key touched by: " .. tostring(otherPart.Name))

	-- Check if it's the player
	local player = game.Players:GetPlayerFromCharacter(otherPart.Parent)
	if player then
		print("Player collected the key!")
		key.Transparency = 1
		key.CanTouch = false
	end
end)
"""
		var result = sbx_runtime.execute_script(key_script)
		print("Key script result: ", result)

	# Run the door script
	var door_trigger = $Room1_Key/Door1/TriggerArea as SbxPart
	if door_trigger:
		var door_script = """
local trigger = script.Parent
local door = trigger.Parent
print("Door script loaded")

trigger.Touched:Connect(function(otherPart)
	print("Door trigger touched")
	local player = game.Players:GetPlayerFromCharacter(otherPart.Parent)
	if player then
		print("Player at door - opening!")
		-- Move door up to open
		local doorPart = door:FindFirstChild("DoorPart")
		if doorPart then
			doorPart.Position = doorPart.Position + Vector3.new(0, 4, 0)
			doorPart.CanCollide = false
		end
	end
end)
"""
		var result = sbx_runtime.execute_script(door_script)
		print("Door script result: ", result)

	# Test basic Luau execution
	_test_luau_basics()

func _test_luau_basics() -> void:
	print("\n=== Testing Luau Integration ===")

	# Test 1: Basic script execution
	var result1 = sbx_runtime.execute_script("print('Hello from Luau!')")
	print("Test 1 (print): ", result1)

	# Test 2: Access game global
	var result2 = sbx_runtime.execute_script("""
print("Game class: " .. game.ClassName)
print("Workspace class: " .. workspace.ClassName)
""")
	print("Test 2 (game/workspace): ", result2)

	# Test 3: Access Players service
	var result3 = sbx_runtime.execute_script("""
local Players = game:GetService("Players")
print("Players service: " .. Players.ClassName)
local localPlayer = Players.LocalPlayer
if localPlayer then
	print("Local player: " .. localPlayer.DisplayName)
else
	print("No local player yet")
end
""")
	print("Test 3 (Players): ", result3)

	# Test 4: RunService signals
	var result4 = sbx_runtime.execute_script("""
local RunService = game:GetService("RunService")
local count = 0
RunService.Heartbeat:Connect(function(dt)
	count = count + 1
	if count <= 3 then
		print("Heartbeat #" .. count .. " dt=" .. string.format("%.3f", dt))
	end
end)
print("RunService.Heartbeat connected!")
""")
	print("Test 4 (RunService): ", result4)

	# Test 5: Create Part via Luau
	var result5 = sbx_runtime.execute_script("""
local testPart = Instance.new("Part")
testPart.Name = "LuauCreatedPart"
testPart.Size = Vector3.new(2, 2, 2)
testPart.Position = Vector3.new(5, 5, 5)
testPart.Parent = workspace
print("Created Part: " .. testPart.Name .. " in workspace")
""")
	print("Test 5 (Instance.new): ", result5)

	print("=== Luau Integration Tests Complete ===\n")
