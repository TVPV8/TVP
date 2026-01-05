function isPremiumArea(position)
	local x = position.x / 32
	local y = position.y / 32

	if x <= 1035 then
		if x <= 1031 or y > 998 then
			if x <= 1029 or y <= 1007 then
				if x <= 1028 or y <= 1008 then
					if x <= 1021 or y <= 1013 then
						return y >= 1016
					end
				end
			end
		end
	end

	return true
end

local conditionLight = Condition(CONDITION_LIGHT)
conditionLight:setTicks(-1)
conditionLight:setParameter(CONDITION_PARAM_LIGHT_LEVEL, 9)

local creatureevent = CreatureEvent("PlayerLogin")

function creatureevent.onLogin(player)
	local loginStr = ""
	if player:getLastLoginSaved() <= 0 then
		loginStr = string.format("Welcome to %s! Please choose your outfit.", configManager.getString(configKeys.SERVER_NAME))
		player:sendOutfitWindow()
	else
		if loginStr ~= "" then
			player:sendTextMessage(MESSAGE_STATUS_DEFAULT, loginStr)
		end

		loginStr = string.format(
		"Your last visit in %s: %s.",
		configManager.getString(configKeys.SERVER_NAME),
		os.date("%d. %b %Y %X %Z", player:getLastLoginSaved())
	)
	end
	player:sendTextMessage(MESSAGE_STATUS_DEFAULT, loginStr)

	-- Stamina
	nextUseStaminaTime[player.uid] = 0

	-- Promotion
	local isPromoted = player:getStorageValue(PlayerStorageKeys.promotion)
	local vocation = player:getVocation()
	local promotion = vocation:getPromotion()
	if player:isPremium() then
		if isPromoted == 1 then
			player:setVocation(promotion)
		end
	elseif isPromoted == 1 and not player:isPremium() then
		-- Demote the player since we are no longer premium
		player:setVocation(vocation:getDemotion())
	end

	local ClearPremiumOutfits = false
	if ClearPremiumOutfits then
		-- Restore default citizen outfit if player is wearing a premium only outfit
		local currentOutfit = Outfit(player:getOutfit().lookType)
		if currentOutfit and currentOutfit.premium > 0 and not player:isPremium() then
			if player:getSex() == PLAYERSEX_MALE then
				player:setOutfit({lookType=128, lookHead=78, lookBody=106, lookLegs=58, lookFeet=95})
			else
				player:setOutfit({lookType=136, lookHead=78, lookBody=106, lookLegs=58, lookFeet=95})
			end
		end
	end

	-- Player lost premium time
	local lostPremiumRights = player:getPremiumEndsAt() ~= 0 and 
	player:getLastLoginSaved() ~= 0 and player:getLastLoginSaved() <= 
	player:getPremiumEndsAt() and player:getPremiumEndsAt() <= os.time()

	if isPremiumArea(player:getPosition()) then
		if player:getVocation():getId() > 0 and lostPremiumRights then
			if player:getLastLogout() <= player:getPremiumEndsAt() and player:getPremiumEndsAt() <= os.time() then
				local town = Town("Thais")
				if town then
					player:teleportTo(town:getTemplePosition())
					player:setTown(town)
					print("Player " .. player:getName() .. " lost premium time and was teleported to Thais.")
				end
			end
		elseif player:getVocation():getId() == 0 and lostPremiumRights then
			if player:getLastLogout() <= player:getPremiumEndsAt() and player:getPremiumEndsAt() <= os.time() then
				local town = Town("Rookgaard")
				if town then
					player:teleportTo(town:getTemplePosition())
					player:setTown(town)
					print("Player " .. player:getName() .. " lost premium time and was teleported to Rookgaard (newbie).")
				end
			end
		end
	end
	
	-- Gamemaster Light
	if player:getGroup():hasFlag(PlayerFlag_FullLight) then
		player:addCondition(conditionLight)
	end

	-- Events
	player:registerEvent("PlayerDeath")
	player:registerEvent("DropLoot")
	return true
end

creatureevent:register()