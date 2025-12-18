local talkaction = TalkAction("!buyhouse")

local config = {
	level = 1,
	disableBuyhouse = false
}

function talkaction.onSay(player, words, param)
	if config.disableBuyhouse then
		player:sendTextMessage(MESSAGE_INFO_DESCR, "Buying houses in game is currently disabled.")
		return false
	end
		
	if player:getLevel() < config.level then
		player:sendCancelMessage("You need level " .. config.level .. " or higher to buy a house.")
		return false
	end

	if configManager.getBoolean(configKeys.HOUSES_ONLY_PREMIUM) and not player:isPremium() then
		player:sendCancelMessage("You need a premium account in order to buy a house.")
		return false
	end

	local position = player:getPosition()
	position:getNextPosition(player:getDirection())

	local tile = Tile(position)
	local house = tile and tile:getHouse()
	if not house then
		player:sendCancelMessage("You have to be looking at the door of the house you would like to buy.")
		return false
	end

	if configManager.getBoolean(configKeys.GUILHALLS_ONLYFOR_LEADERS) then
		if house:isGuildHall() then
			if not player:getGuild() or player:getGuildLevel() < 3 then
				player:sendCancelMessage("Only leaders of a guild can purchase a guildhall.")
				return false
			end
		end
	end

	if house:getOwnerGuid() > 0 then
		player:sendCancelMessage("This house already has an owner.")
		return false
	end

	if player:getHouse() then
		player:sendCancelMessage("You are already the owner of a house.")
		return false
	end

	local price = house:getRent()	
	local housePrice = configManager.getNumber(configKeys.HOUSE_PRICE)
	if housePrice > -1 then 
		price = house:getTileCount() * housePrice
	end
	
	if not player:removeTotalMoney(price) then
		player:sendCancelMessage("You do not have enough money.")
		return false
	end

	house:setOwnerGuid(player:getGuid())
	local townName = house:getTown():getName()
	player:sendTextMessage(MESSAGE_INFO_DESCR, "You have successfully bought this house, be sure to have the money for the rent in the depot of " .. townName .. ".")
	return false
end

talkaction:separator(" ")
talkaction:register()