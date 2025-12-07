-- Pressure Plate Script
-- Attaches to a Part that opens a door when stepped on

local plate = script.Parent
local door = plate:GetAttribute("Door")
local stayOpen = plate:GetAttribute("StayOpen") or false

local isPressed = false
local playersOnPlate = {}

-- Function to update door state
local function updateDoor(open)
	local doorModel = workspace:FindFirstChild(door)
	if not doorModel then return end

	if open then
		if doorModel.ForceOpen then
			doorModel:ForceOpen()
		end
	end
	-- If not stayOpen, door would close when player leaves
end

-- When player steps on plate
plate.Touched:Connect(function(otherPart)
	local player = game.Players:GetPlayerFromCharacter(otherPart.Parent)
	if not player then return end

	-- Track this player
	if not playersOnPlate[player] then
		playersOnPlate[player] = true
		print("Player stepped on pressure plate")

		-- Visual feedback - plate goes down
		plate.Position = plate.Position - Vector3.new(0, 0.05, 0)

		if not isPressed then
			isPressed = true
			updateDoor(true)
		end
	end
end)

-- When player leaves plate
plate.TouchEnded:Connect(function(otherPart)
	local player = game.Players:GetPlayerFromCharacter(otherPart.Parent)
	if not player then return end

	-- Remove this player
	if playersOnPlate[player] then
		playersOnPlate[player] = nil
		print("Player left pressure plate")

		-- Check if any players still on plate
		local anyOnPlate = false
		for _ in pairs(playersOnPlate) do
			anyOnPlate = true
			break
		end

		if not anyOnPlate and not stayOpen then
			isPressed = false
			-- Door would close here if not stayOpen
			plate.Position = plate.Position + Vector3.new(0, 0.05, 0)
		end
	end
end)
