-- Button Puzzle Script
-- Attaches to a Model containing buttons that must be pressed in sequence

local puzzle = script.Parent
local correctSequence = {1, 2, 3}  -- Red, Blue, Green
local door = puzzle:GetAttribute("Door")

local pressedSequence = {}
local solved = false

-- Find all button parts
local buttons = {}
for _, child in ipairs(puzzle:GetChildren()) do
	if child:GetAttribute("ButtonId") then
		local buttonId = child:GetAttribute("ButtonId")
		buttons[buttonId] = child
	end
end

-- Function to reset the puzzle
local function resetPuzzle()
	pressedSequence = {}
	for id, button in pairs(buttons) do
		-- Reset button visual
		button.Position = button.Position  -- Trigger update
	end
	print("Puzzle reset - try again!")
end

-- Function to check if sequence is correct
local function checkSequence()
	for i, expectedId in ipairs(correctSequence) do
		if pressedSequence[i] ~= expectedId then
			return false
		end
	end
	return #pressedSequence == #correctSequence
end

-- Setup each button
for buttonId, button in pairs(buttons) do
	button.Touched:Connect(function(otherPart)
		if solved then return end

		local player = game.Players:GetPlayerFromCharacter(otherPart.Parent)
		if not player then return end

		-- Add to sequence
		table.insert(pressedSequence, buttonId)
		print("Button " .. buttonId .. " pressed")

		-- Visual feedback - press animation
		local originalY = button.Position.Y
		button.Position = button.Position - Vector3.new(0, 0.1, 0)

		-- Check if sequence is complete
		if #pressedSequence == #correctSequence then
			if checkSequence() then
				solved = true
				print("Puzzle solved!")

				-- Open the door
				local doorModel = workspace:FindFirstChild(door)
				if doorModel and doorModel.ForceOpen then
					doorModel:ForceOpen()
				end
			else
				-- Wrong sequence, reset after short delay
				task.delay(0.5, resetPuzzle)
			end
		end
	end)
end
