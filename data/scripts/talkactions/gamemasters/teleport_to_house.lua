local talkaction = TalkAction("/gotohouse")

function talkaction.onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	if not param or param == "" then
		player:sendCancelMessage("House name or ID not found.")
		return false
	end

	local houseList = Game.getHouses()
	for _, house in ipairs(Game.getHouses()) do
		if house:getId() == tonumber(param) or house:getName():lower() == param:lower() then
			local position = house:getExitPosition()
			if position then
				player:teleportTo(position)
				return false
			end
		end
	end

	player:sendCancelMessage("House name or ID not found.")
	return false
end

talkaction:separator(" ")
talkaction:register()