-- Key Script
-- Attaches to a Part named "Key" that players can collect

local key = script.Parent
local keyName = key.Name

-- Track if key has been collected
local collected = false

-- When player touches the key, collect it
key.Touched:Connect(function(otherPart)
	if collected then return end

	-- Check if the touching part is a player's character
	local player = game.Players:GetPlayerFromCharacter(otherPart.Parent)
	if not player then return end

	collected = true

	-- Add key to player's inventory (stored as attribute on player)
	local inventory = player:GetAttribute("Inventory") or ""
	player:SetAttribute("Inventory", inventory .. keyName .. ",")

	-- Visual feedback - make key disappear
	key.Transparency = 1
	key.CanCollide = false
	key.CanTouch = false

	print("Player collected: " .. keyName)
end)
