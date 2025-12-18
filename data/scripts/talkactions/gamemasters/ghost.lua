local talkaction = TalkAction("/ghost", "/g")

function talkaction.onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	local position = player:getPosition()
	local isGhost = not player:isInGhostMode()

	player:setGhostMode(isGhost)
	if isGhost then
		player:sendTextMessage(MESSAGE_INFO_DESCR, "You are now invisible.")
		player:sendTextMessage(MESSAGE_STATUS_WARNING, "[WARNING] During ghost mode try to avoid walking near stacked creatures as that would result in desynchronization from your client to the server.\nIf you were to desync relogin or teleport away.")
		position:sendMagicEffect(CONST_ME_POFF)
	else
		player:sendTextMessage(MESSAGE_INFO_DESCR, "You are visible again.")
		position:sendMagicEffect(CONST_ME_TELEPORT)
	end
	return false
end

talkaction:separator(" ")
talkaction:register()