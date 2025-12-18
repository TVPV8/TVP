local ec = EventCallback

ec.onLook = function(self, thing, position, distance, description)
	local description = "You see " .. thing:getDescription(distance)
	if self:getGroup():getAccess() then
		if thing:isItem() then
			description = string.format("%s\nItem ID: %d", description, thing:getId())

			local actionId = thing:getActionId()
			if actionId ~= 0 then
				description = string.format("%s, Action ID: %d", description, actionId)
			end

			local uniqueId = thing:getAttribute(ITEM_ATTRIBUTE_UNIQUEID)
			if uniqueId > 0 and uniqueId < 65536 then
				description = string.format("%s, Unique ID: %d", description, uniqueId)
			end

			local itemType = thing:getType()

			local transformEquipId = itemType:getTransformEquipId()
			local transformDeEquipId = itemType:getTransformDeEquipId()
			if transformEquipId ~= 0 then
				description = string.format("%s\nTransforms to: %d (onEquip)", description, transformEquipId)
			elseif transformDeEquipId ~= 0 then
				description = string.format("%s\nTransforms to: %d (onDeEquip)", description, transformDeEquipId)
			end

			local decayId = itemType:getDecayId()
			if decayId ~= -1 then
				description = string.format("%s\nDecays to: %d", description, decayId)
			end
			
			local doorQuestNumber = thing:getAttribute(ITEM_ATTRIBUTE_DOORQUESTNUMBER)
			if doorQuestNumber ~= 0 then
				description = string.format("%s\nDoor Quest Number: %d", description, doorQuestNumber)
				description = string.format("%s\nDoor Quest Value: %d", description, thing:getAttribute(ITEM_ATTRIBUTE_DOORQUESTVALUE))
			end
			
			local keyholeNumber = thing:getAttribute(ITEM_ATTRIBUTE_KEYHOLENUMBER)
			if keyholeNumber ~= 0 then
				description = string.format("%s\nKeyHole Number: %d", description, keyholeNumber)
			end
		elseif thing:isCreature() then
			local str = "%s\nHealth: %d / %d"
			if thing:isPlayer() and thing:getMaxMana() > 0 then
				str = string.format("%s, Mana: %d / %d", str, thing:getMana(), thing:getMaxMana())
			end
			description = string.format(str, description, thing:getHealth(), thing:getMaxHealth()) .. "."
		end

		local position = thing:getPosition()
		description = string.format(
			"%s\nPosition: %d, %d, %d",
			description, position.x, position.y, position.z
		)
		
		local tile = thing:getTile()
		if tile:hasFlag(TILESTATE_REFRESH) then
			description = string.format("%s\nRefreshable zone.", description)
		end
		
		if tile:hasFlag(TILESTATE_NOLOGOUT) then
			description = string.format("%s\nNo-Logout zone.", description)
		end
				
		if tile:hasFlag(TILESTATE_PVPZONE) then
			description = string.format("%s\nPvP-Zone.", description)
		end
		
		if tile:hasFlag(TILESTATE_PROTECTIONZONE) then
			description = string.format("%s\nProtection zone.", description)
		end
		
		if thing:isCreature() then
			if thing:isPlayer() then
				description = string.format("%s\nIP: %s.", description, Game.convertIpToString(thing:getIp()))
			end
		end
	end
	return description
end

ec:register()
