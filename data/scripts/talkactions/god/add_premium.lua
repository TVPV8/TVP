local talkaction = TalkAction("/addpremiumdays")

function talkaction.onSay(player, words, param, type)
	if not player:getGroup():getAccess() then
		return true
	end

	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	local params = string.split(param, ",")
	if #params < 2 then
		player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, "Not enough parameters given.\nUse as: target,premium days")
		return false
	end

	local premiumDays = tonumber(params[2])
	if not premiumDays or premiumDays == 0 then
		player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, "Invalid amount of days.")
		return false
	end
	
	local target = Player(params[1])
	if not target then
		player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, "Player not found.")
		return false
	end
	
	if target:getPremiumEndsAt() <= os.time() then
		target:setPremiumEndsAt(os.time() + (86400 * tonumber(premiumDays)))
	else
		target:setPremiumEndsAt(target:getPremiumEndsAt() + (86400 * tonumber(premiumDays)))
	end
	target:sendTextMessage(MESSAGE_INFO_DESCR, "You have been gifted " .. premiumDays .. " days of premium by " .. player:getName() .. ".")
	target:getPosition():sendMagicEffect(CONST_ME_YELLOW_RINGS)
	return false
end

talkaction:access(true)
talkaction:separator(" ")
talkaction:register()