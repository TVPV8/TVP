local talkaction = TalkAction("/save")

function talkaction.onSay(player, words, param, type)
	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	saveServer()
	player:sendTextMessage(MESSAGE_INFO_DESCR, "Game state saved.")
	return false
end

talkaction:access(true)
talkaction:separator(" ")
talkaction:register()