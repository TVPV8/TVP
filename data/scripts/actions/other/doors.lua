local positionOffsets = {
	Position(1, 0, 0), -- east
	Position(0, 1, 0), -- south
}

local door = Action()

function door.onUse(player, item, fromPosition, target, toPosition)
	local itemId = item:getId()
	if table.contains(closedQuestDoors, itemId) then
		local questDoorValue = item:getAttribute(ITEM_ATTRIBUTE_DOORQUESTVALUE)
		if questDoorValue == 0 then
			questDoorValue = -1
		end
		if player:getStorageValue(item:getAttribute(ITEM_ATTRIBUTE_DOORQUESTNUMBER)) == questDoorValue or player:getGroup():getAccess() then
			item:transform(itemId + 1)
			player:teleportTo(toPosition, true)
		else
			player:sendTextMessage(MESSAGE_INFO_DESCR, "The door seems to be sealed against unwanted intruders.")
		end
		return true
	elseif table.contains(closedLevelDoors, itemId) then
		if item:hasAttribute(ITEM_ATTRIBUTE_DOORLEVEL) and player:getLevel() >= item:getAttribute(ITEM_ATTRIBUTE_DOORLEVEL) or player:getGroup():getAccess() then
			item:transform(itemId + 1)
			player:teleportTo(toPosition, true)
		else
			player:sendTextMessage(MESSAGE_INFO_DESCR, "Only the worthy may pass.")
		end
		return true
	elseif table.contains(keys, itemId) then
		local tile = Tile(toPosition)
		if not tile then
			return false
		end
		target = tile:getTopVisibleThing()
		if not target:isItem() then
			return false
		end

		if table.contains(keys, target.itemid) then
			return false
		end
		
		if not table.contains(openDoors, target.itemid) and not table.contains(closedDoors, target.itemid) and not table.contains(lockedDoors, target.itemid) then
			return false
		end
				
		if not target:hasAttribute(ITEM_ATTRIBUTE_KEYHOLENUMBER) then
			player:sendTextMessage(MESSAGE_STATUS_SMALL, "The key does not match.")
			return true
		end
		
		if item:getAttribute(ITEM_ATTRIBUTE_KEYNUMBER) ~= target:getAttribute(ITEM_ATTRIBUTE_KEYHOLENUMBER) then
			player:sendTextMessage(MESSAGE_STATUS_SMALL, "The key does not match.")
			return true
		end
		local transformTo = target.itemid + 2
		if table.contains(openDoors, target.itemid) then
			transformTo = target.itemid - 2
		elseif table.contains(closedDoors, target.itemid) then
			transformTo = target.itemid - 1
		end
		target:transform(transformTo)
		return true
	elseif table.contains(lockedDoors, itemId) then
		player:sendTextMessage(MESSAGE_INFO_DESCR, "It is locked.")
		return true	
	elseif table.contains(openDoors, itemId) or table.contains(openExtraDoors, itemId) or table.contains(openHouseDoors, itemId) then
		local tile = Tile(toPosition)
		local doorCreatures = tile:getCreatures()
		if doorCreatures and #doorCreatures > 0 then
			local leftOffset = Position(-1, 0, 0)
			local rightOffset = Position(1, 0, 0)
			local southOffset = Position(0, 1, 0)
			local northOffset = Position(0, -1, 0)

			local isHorizontalDoor = Tile(item:getPosition() + leftOffset):hasFlag(TILESTATE_IMMOVABLEBLOCKSOLID) == false

			for _, doorCreature in pairs(doorCreatures) do
				if isHorizontalDoor then
					local rightTile = Tile(item:getPosition() + rightOffset)

					if not rightTile:hasFlag(TILESTATE_IMMOVABLEBLOCKSOLID) then
						doorCreature:teleportTo(rightTile:getPosition(), true)
					elseif Tile(item:getPosition() + southOffset):hasFlag(TILESTATE_IMMOVABLEBLOCKSOLID) == false then
						doorCreature:teleportTo(item:getPosition() + southOffset, true)
					end
				else
					local southTile = Tile(item:getPosition() + southOffset)

					if not southTile:hasFlag(TILESTATE_IMMOVABLEBLOCKSOLID) then
						doorCreature:teleportTo(southTile:getPosition(), true)
					end
				end
			end
		end
		
		local magicField = tile:getItemByType(ITEM_TYPE_MAGICFIELD)
		if magicField then
			magicField:remove()
		end

		item:transform(itemId - 1)
		return true	
	elseif table.contains(closedDoors, itemId) or table.contains(closedExtraDoors, itemId) or table.contains(closedHouseDoors, itemId) then
		item:transform(itemId + 1)
		return true
	end
	return false
end

local doorTables = {keys, openDoors, closedDoors, lockedDoors, openExtraDoors, closedExtraDoors, openHouseDoors, closedHouseDoors, closedQuestDoors, closedLevelDoors}
for _, doors in pairs(doorTables) do
	for _, doorId in pairs(doors) do
		door:id(doorId)
	end
end
door:register()
