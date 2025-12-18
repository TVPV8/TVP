local talkaction = TalkAction("/ipban")

local ipBanDays = 7

local function kickPlayersFromIP(ip)
	for _, player in pairs(Game.getPlayers()) do
		if player:getIp() == ip then
			player:remove()
		end
	end
end

function talkaction.onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	local targetName = ""
	local targetIp = 0

	local targetPlayer = Player(param)
	if targetPlayer then
		targetIp = targetPlayer:getIp()
		targetName = targetPlayer:getName()
	else
		local resultId = db.storeQuery("SELECT `name`, `lastip` FROM `players` WHERE `name` = " .. db.escapeString(param))
		if resultId == false then
			return false
		end

		targetName = result.getString(resultId, "name")
		targetIp = result.getNumber(resultId, "lastip")
		result.free(resultId)
	end

	if targetIp == 0 then
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "No IP available to be banned.")
		return false
	end

	local timeNow = os.time()
	db.query("INSERT INTO `ip_bans` (`ip`, `reason`, `banned_at`, `expires_at`, `banned_by`) VALUES (" ..
			targetIp .. ", '', " .. timeNow .. ", " .. timeNow + (ipBanDays * 86400) .. ", " .. player:getGuid() .. ")")
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, targetName .. "  has been IP banned.")
	kickPlayersFromIP(targetIp)
	return false
end

talkaction:separator(" ")
talkaction:register()