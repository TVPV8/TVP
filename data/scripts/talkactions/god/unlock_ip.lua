local talkaction = TalkAction("/unlockip")

function talkaction.onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	local o1,o2,o3,o4 = param:match("(%d%d?%d?)%.(%d%d?%d?)%.(%d%d?%d?)%.(%d%d?%d?)")
	if o1 == nil or o2 == nil or o3 == nil or o4 == nil then
		player:sendCancelMessage("Type in a valid IP Address (0.0.0.0)")
		return false
	end

	local ip = getIPNumberFromString(param)
	if ip <= 0 then
		player:sendCancelMessage("Type in a valid IP Address (0.0.0.0)")
		return false
	end
	
	Game.unlockIp(tonumber(ip))
	player:sendCancelMessage("The IP-Address " .. param .. " has been unlocked.")
	return false
end

talkaction:separator(" ")
talkaction:register()