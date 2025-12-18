local creatureevent = CreatureEvent("PlayerLeaveGame")

function creatureevent.onLeaveGame(player)
	local playerId = player:getId()
	if nextUseStaminaTime[playerId] then
		nextUseStaminaTime[playerId] = nil
	end
	return true
end

creatureevent:register()