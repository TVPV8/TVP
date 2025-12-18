local talkaction = TalkAction("!kills")

function talkaction.onSay(player, words, param)
	if Game.getWorldType() == WORLD_TYPE_PVP_ENFORCED then
		player:showTextDialog(2598, "Your character has no murder history.", false)
		return false
	end
	
	local today = os.time()
	local lastDay = 0
	local lastWeek = 0
	local lastMonth = 0
	local eligibleMurders = 0
	local dayTimestamp = today - (24 * 60 * 60)
	local weekTimestamp = today - (7 * 24 * 60 * 60)
	local monthTimestamp = today - (30 * 24 * 60 * 60)
	
	local killsDayRedSkull = configManager.getNumber(configKeys.KILLS_DAY_RED_SKULL)
	local killsWeekRedSkull = configManager.getNumber(configKeys.KILLS_WEEK_RED_SKULL)
	local killsMonthRedSkull = configManager.getNumber(configKeys.KILLS_MONTH_RED_SKULL)
	
	local killsDayBanishment = configManager.getNumber(configKeys.KILLS_DAY_BANISHMENT)
	local killsWeekBanishment = configManager.getNumber(configKeys.KILLS_WEEK_BANISHMENT)
	local killsMonthBanishment = configManager.getNumber(configKeys.KILLS_MONTH_BANISHMENT)
	
	for _, timestamp in pairs(player:getMurderTimestamps()) do
		if timestamp > dayTimestamp then
			lastDay = lastDay + 1
		end
		
		if timestamp > weekTimestamp then
			lastWeek = lastWeek + 1
		end
		
		eligibleMurders = lastMonth + 1
		
		if timestamp <= monthTimestamp then
			eligibleMurders = lastMonth
		end
		
		lastMonth = eligibleMurders
	end
	
	local message = ""
	message = message .. "Default murders\n"
	message = message .. "- Daily kills for red skull " .. killsDayRedSkull .. "\n"
	message = message .. "- Weekly kills for red skull " .. killsWeekRedSkull .. "\n"
	message = message .. "- Monthly kills for red skull " .. killsMonthRedSkull .. "\n"
	
	message = message .. "- Daily kills for banishment " .. killsDayBanishment .. "\n"
	message = message .. "- Weekly kills for banishment " .. killsWeekBanishment .. "\n"
	message = message .. "- Monthly kills for banishment " .. killsMonthBanishment .. "\n"
	
	message = message .. "\n"
	
	message = message .. "Last murders within 24 hours " .. lastDay .. "\n"
	message = message .. "Last murders within a week " .. lastWeek .. "\n"
	message = message .. "Last murders within a month " .. lastMonth .. "\n"
	
	message = message .. "\n"
	
	message = message .. "Players you may kill for a red skull:\n"
	
	if player:getSkull() == SKULL_RED then
		message = message .. "- Your character already has red skull.\n"
	else
		message = message .. "- Within 24 hours " .. killsDayRedSkull - lastDay .. " murders.\n"
		message = message .. "- Within a week " .. killsWeekRedSkull - lastWeek .. " murders.\n"
		message = message .. "- Within a month " .. killsMonthRedSkull - lastMonth .. " murders.\n"
	end
	
	message = message .. "\n"
	
	message = message .. "Players you may kill for a banishment:\n"
	message = message .. "- Within 24 hours " .. killsDayBanishment - lastDay .. " murders.\n"
	message = message .. "- Within a week " .. killsWeekBanishment - lastWeek .. " murders.\n"
	message = message .. "- Within a month " .. killsMonthBanishment - lastDay .. " murders.\n"
			
	player:showTextDialog(2598, message, false)
	return false
end

talkaction:register()