local talkaction = TalkAction("omani")

function talkaction.onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	local target = Player(param)
	if not target then
		player:sendCancelMessage("Player not found.")
		return false
	end

	target:teleportTo(target:getTown():getTemplePosition())
	target:getPosition():sendMagicEffect(CONST_ME_TELEPORT)
	return false
end

talkaction:separator(" ")
talkaction:register()