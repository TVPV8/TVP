local talkaction = TalkAction("!sellhouse")

function talkaction.onSay(player, words, param)
	local tradePartner = Player(param)
	if not tradePartner or tradePartner == player then
		player:sendCancelMessage("Trade player not found.")
		return false
	end

	local house = player:getTile():getHouse()
	if not house then
		player:sendCancelMessage("You must stand in your house to initiate the trade.")
		return false
	end

	if not configManager.getBoolean(configKeys.HOUSE_TRANSFEROWNERSHIP_TRANSFERITEMS) then
		player:sendTextMessage(MESSAGE_STATUS_WARNING, "Transfering your house ownership does not clean up your house items.")
	end

	local returnValue = house:startTrade(player, tradePartner)
	if returnValue ~= RETURNVALUE_NOERROR then
		player:sendCancelMessage(returnValue)
	end
	return false
end

talkaction:separator(" ")
talkaction:register()