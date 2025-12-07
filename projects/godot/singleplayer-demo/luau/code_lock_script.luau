-- Code Lock Script
-- Attaches to a Model with a code lock that opens a door when correct code entered

local codeLock = script.Parent
local correctCode = codeLock:GetAttribute("CorrectCode") or "2048"
local door = codeLock:GetAttribute("Door")

local currentInput = ""
local solved = false

-- Function to submit code
local function submitCode(code)
	if solved then return end

	if code == correctCode then
		solved = true
		print("Correct code! Door unlocking...")

		-- Visual feedback
		local display = codeLock:FindFirstChild("Display")
		if display then
			-- Update display to show success
		end

		-- Open the door
		local doorModel = workspace:FindFirstChild(door)
		if doorModel and doorModel.ForceOpen then
			doorModel:ForceOpen()
		end
	else
		print("Wrong code: " .. code)
		currentInput = ""
	end
end

-- When player interacts with code lock
local triggerArea = codeLock:FindFirstChild("TriggerArea")
if triggerArea then
	triggerArea.Touched:Connect(function(otherPart)
		local player = game.Players:GetPlayerFromCharacter(otherPart.Parent)
		if not player then return end

		-- In a full implementation, this would open a GUI for code entry
		-- For demo purposes, we auto-submit the correct code when touched
		print("Code lock activated - enter code 2048")
	end)
end

-- Keyboard input handling would go here in full implementation
-- For now, expose a function for external code entry
function codeLock:EnterCode(code)
	submitCode(code)
end
