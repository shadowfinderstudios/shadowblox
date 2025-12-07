-- Door Script
-- Attaches to a Model with a door Part that opens when conditions are met

local door = script.Parent
local doorPart = door:FindFirstChild("DoorPart")
local requiredKey = door:GetAttribute("RequiredKey") or ""

local isOpen = false

-- Function to open the door
local function openDoor()
	if isOpen then return end
	isOpen = true

	-- Animate door opening by moving it up
	local originalPos = doorPart.Position
	doorPart.Position = originalPos + Vector3.new(0, 4, 0)
	doorPart.CanCollide = false

	print("Door opened!")
end

-- Function to check if player has required key
local function hasRequiredKey(player)
	if requiredKey == "" then return true end

	local inventory = player:GetAttribute("Inventory") or ""
	return string.find(inventory, requiredKey)
end

-- When player touches door trigger area
local triggerArea = door:FindFirstChild("TriggerArea")
if triggerArea then
	triggerArea.Touched:Connect(function(otherPart)
		local player = game.Players:GetPlayerFromCharacter(otherPart.Parent)
		if not player then return end

		if hasRequiredKey(player) then
			openDoor()
		else
			print("You need: " .. requiredKey)
		end
	end)
end

-- Public function to force open (for puzzles)
function door:ForceOpen()
	openDoor()
end
