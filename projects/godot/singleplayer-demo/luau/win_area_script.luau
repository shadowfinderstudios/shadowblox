-- Win Area Script
-- Attaches to a Part that triggers win condition when player enters

local winArea = script.Parent
local hasWon = false

winArea.Touched:Connect(function(otherPart)
	if hasWon then return end

	local player = game.Players:GetPlayerFromCharacter(otherPart.Parent)
	if not player then return end

	hasWon = true

	print("========================================")
	print("  CONGRATULATIONS!")
	print("  You escaped the room!")
	print("  " .. player.DisplayName .. " wins!")
	print("========================================")

	-- Visual celebration could go here
	-- Confetti particles, sounds, etc.

	-- Teleport player back to start or show win screen
	local character = player.Character
	if character then
		local root = character:FindFirstChild("HumanoidRootPart")
		if root then
			root.Position = Vector3.new(0, 3, 0)
		end
	end
end)
