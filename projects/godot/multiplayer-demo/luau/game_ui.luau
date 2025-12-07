-- GameUI (StarterGui/GameUI LocalScript)
-- Client-side script for updating the game status UI

local ReplicatedStorage = game:GetService("ReplicatedStorage")
local Players = game:GetService("Players")

local gameStatus = ReplicatedStorage:WaitForChild("GameStatus")
local remoteEvents = ReplicatedStorage:WaitForChild("RemoteEvents")
local infectionEvent = remoteEvents:WaitForChild("InfectionEvent")
local gameStateEvent = remoteEvents:WaitForChild("GameStateEvent")

local localPlayer = Players.LocalPlayer

print("[GameUI] Client UI initialized")

-- Listen for game status changes via Changed signal
gameStatus.Changed:Connect(function()
	print("[GameUI] Status: " .. gameStatus.Value)
	-- In a full implementation, this would update a TextLabel
end)

-- Listen for infection events
infectionEvent.OnClientEvent:Connect(function(infectedPlayer)
	if infectedPlayer == localPlayer then
		print("[GameUI] YOU ARE INFECTED! Tag others to spread the infection!")
	else
		print("[GameUI] " .. infectedPlayer.Name .. " has been infected!")
	end
end)

-- Listen for game state events
gameStateEvent.OnClientEvent:Connect(function(status)
	print("[GameUI] Game State: " .. status)
end)

-- Initial status
print("[GameUI] Initial status: " .. gameStatus.Value)
