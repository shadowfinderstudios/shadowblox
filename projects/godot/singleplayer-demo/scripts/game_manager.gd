extends Node3D
## ShadowBlox Game Manager
## Demonstrates the Luau integration with Godot for the escape room demo
## Falls back to GDScript if the extension isn't available

var sbx_runtime = null  # Will be SbxRuntime if available
var sbx_available := false

func _ready() -> void:
	# Check if SbxRuntime is available (extension loaded)
	if ClassDB.class_exists("SbxRuntime"):
		sbx_available = true
		sbx_runtime = get_node_or_null("SbxRuntime")
		if sbx_runtime:
			print("[GameManager] ShadowBlox extension loaded - using Luau integration")
			await get_tree().process_frame
			_setup_luau_game()
		else:
			print("[GameManager] SbxRuntime node not found in scene")
			sbx_available = false
	else:
		print("[GameManager] ShadowBlox extension not available for this platform")
		print("[GameManager] To build for Windows: scons platform=windows target=template_debug")
		print("[GameManager] Falling back to GDScript implementation")

func _setup_luau_game() -> void:
	# Create the local player
	sbx_runtime.create_local_player(1, "Player1")

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

	# Test 6: Part.Touched event
	var result6 = sbx_runtime.execute_script("""
local testPart = workspace:FindFirstChild("LuauCreatedPart")
if testPart then
	testPart.Touched:Connect(function(otherPart)
		print("LuauCreatedPart touched by: " .. otherPart.Name)
	end)
	print("Touched event connected!")
end
""")
	print("Test 6 (Part.Touched): ", result6)

	print("=== Luau Integration Tests Complete ===\n")
