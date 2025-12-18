local talkaction = TalkAction("/gotopos")

function talkaction.onSay(player, words, param, type)
	local params = string.split(param, ",")
	if #params < 3 then
		player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, "Not enough parameters given.\nUse as: xxxx,yyyy,zz")
		return false
	end

	player:teleportTo({x = tonumber(params[1]), y = tonumber(params[2]), z = tonumber(params[3])}, true)
	return false
end

talkaction:access(true)
talkaction:separator(" ")
talkaction:register()