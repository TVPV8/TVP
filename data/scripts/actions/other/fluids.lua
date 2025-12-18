local config = {
	createEmptyVial = true,
}

local items = {
	1775, 2005, 2006, 2007, 2008, 2009,
	2011, 2012, 2013, 2014, 2015, 2023,
	2031, 2032, 2033, 2034, 2574, 2575,
	2576, 2577, 2562
}

local drunk = Condition(CONDITION_DRUNK)
drunk:setParameter(CONDITION_PARAM_TICKS, 60000)
drunk:setParameter(CONDITION_PARAM_CYCLE, 1)
drunk:setParameter(CONDITION_PARAM_COUNT, 120)
drunk:setParameter(CONDITION_PARAM_MAX_COUNT, 120)

local poison = Condition(CONDITION_POISON)
poison:setParameter(CONDITION_PARAM_CYCLE, 200)
poison:setParameter(CONDITION_PARAM_COUNT, 3)
poison:setParameter(CONDITION_PARAM_MAX_COUNT, 3)

local fluidMessage = {
	[FLUID_URINE] = "Urgh!",
	[FLUID_MILK] = "Mmmh.",
	[FLUID_MANAFLUID] = "Aaaah...",
	[FLUID_LIFEFLUID] = "Aaaah...",
	[FLUID_SLIME] = "Urgh!",
	[FLUID_BEER] = "Aah...",
	[FLUID_WINE] = "Aah...",
}

local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if toPosition == player:getPosition() then
        local tile = Tile(toPosition)
        local creature = tile:getBottomCreature()
        if tile:getItemById(1386) or tile:getItemById(3678) then
            target = player
        elseif creature then
            target = creature
        end
    end
	
	if target == player and item.type == FLUID_NONE then
		return true
	end

	if target.itemid == 1 and target:isPlayer() and target.uid == player.uid then
		-- only allow potions use on-self
		if target.uid == player.uid then
			if table.contains({FLUID_WINE, FLUID_BEER}, item.type) then
				player:addCondition(drunk)
			elseif item.type == FLUID_SLIME then
				player:addCondition(poison)
			elseif item.type == FLUID_MANAFLUID then
				target:setEarliestSpellTime(1000)
				target:addMana(math.random(25, 75))
				toPosition:sendMagicEffect(CONST_ME_MAGIC_BLUE)
			elseif item.type == FLUID_LIFEFLUID then
				target:addHealth(math.random(25, 50))
				toPosition:sendMagicEffect(CONST_ME_MAGIC_BLUE)
			end
		
			player:say(fluidMessage[item.type] or "Gulp.", TALKTYPE_SAY)
		else
			Game.createItem(2016, item.type, toPosition):decay()
		end
		
		if configManager.getBoolean(configKeys.REMOVE_POTION_CHARGES) then
			if config.createEmptyVial then
				item:transform(item:getId(), FLUID_NONE)
			else
				item:remove(1)
			end
		end
		return true
	end
	
	local targetItemType = ItemType(target.itemid)
	if targetItemType:getGroup() == ITEM_GROUP_SPLASH then
		local tile = Tile(toPosition)
		if tile then
			target = tile:getGround()
			targetItemType = ItemType(target.itemid)
		end
		
	end
	
	if targetItemType and target.itemid >= 100 then
		if targetItemType:getFluidSource() ~= FLUID_NONE and item:getFluidType() == FLUID_NONE then
			item:transform(item:getId(), targetItemType:getFluidSource())
			return true
		end

		if item:getFluidType() ~= FLUID_NONE and target:getFluidType() == FLUID_NONE and targetItemType:isFluidContainer() then
			target:transform(target:getId(), item:getFluidType())
			item:transform(item:getId(), FLUID_NONE)
			return true
		end

		if targetItemType:isGroundTile() and targetItemType:getFluidSource() ~= FLUID_NONE and targetItemType:getMagicEffect() ~= CONST_ME_NONE then
			item:transform(item:getId(), FLUID_NONE)
			target:getPosition():sendMagicEffect(targetItemType:getMagicEffect())
			return true
		end

		if toPosition.x ~= CONTAINER_POSITION then
			local tile = Tile(toPosition)
			if tile and tile:queryAdd(item) == RETURNVALUE_NOTENOUGHROOM then
				player:sendCancelMessage(Game.getReturnMessage(RETURNVALUE_NOTENOUGHROOM))
				return true
			end
		end
	end

	if item:getFluidType() ~= FLUID_NONE then
		if toPosition.x == CONTAINER_POSITION then
			toPosition = player:getPosition()
		end

		local splash = Game.createItem(2016, item.type)
		local tile = Tile(toPosition)
		if tile and tile:queryAdd(splash) == RETURNVALUE_NOTENOUGHROOM and target.type == FLUID_NONE or tile and tile:hasFlag(TILESTATE_FLOORCHANGE) then
			item:transform(item:getId(), FLUID_NONE)
			player:sendCancelMessage(Game.getReturnMessage(RETURNVALUE_NOTENOUGHROOM))
			return true
		end

		if not tile:getItemByType(ITEM_TYPE_TRASHHOLDER) and tile:addItemEx(splash) == RETURNVALUE_NOERROR then
			splash:decay()
		else
			player:sendCancelMessage(Game.getReturnMessage(RETURNVALUE_NOTENOUGHROOM))
			splash:remove()
		end
		
		if config.createEmptyVial then
			item:transform(item:getId(), FLUID_NONE)
		else
			item:remove(1)
		end

		return true
	end
	return false
end

for _, id in ipairs(items) do
	action:id(id)
end

action:register()
