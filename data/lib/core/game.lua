function Game.setMapItemActionId(position, itemid, actionid)
	local tile = Tile(position)
	if not tile then
		error("Game.setMapItemActionId - Tile not found")
		return false
	end
	
	local item = tile:getItemById(itemid)
	if not item then
		error("Game.setMapItemActionId - Item not found")
		return false
	end
	
	item:setActionId(actionid)
	return true
end

function Game.isItemInPosition(position, itemid)
	local tile = Tile(position)
	if not tile then
		error("Game.isItemInPosition - Tile not found")
		return false
	end

	local item = tile:getItemById(itemid)
	if not item then
		return false
	end
	
	return true
end

function Game.removeItemInPosition(position, itemid)
	local tile = Tile(position)
	if not tile then
		error("Game.removeItemInPosition - Tile not found")
		return false
	end
	
	local item = tile:getItemById(itemid)
	if not item then
		return false
	end
	
	item:remove()
	return true
end

function Game.removeItemsInPosition(position)
	local tile = Tile(position)
	if not tile then
		error("Game.removeItemsInPosition - Tile not found")
		return false
	end
	
	for _, item in ipairs(tile:getItems()) do
		local itemType = ItemType(item:getId())
		if itemType:isMovable() then
			item:remove()
		end
	end
end

function Game.sendMagicEffect(position, effect)
	local pos = Position(position.x, position.y, position.z)
	pos:sendMagicEffect(effect)
end

function Game.transformItemInPosition(position, fromitemid, toitemid)
	local tile = Tile(position)
	if not tile then
		error("Game.transformItemInPosition - Tile not found")
		return false
	end

	local item = tile:getItemById(fromitemid)
	if not item then
		return false
	end
	
	item:transform(toitemid)
	item:decay()
	return true
end

function Game.broadcastMessage(message, messageType)
	if not messageType then
		messageType = MESSAGE_STATUS_WARNING
	end

	for _, player in ipairs(Game.getPlayers()) do
		player:sendTextMessage(messageType, message)
	end
end

function Game.convertIpToString(ip)
	local band = bit.band
	local rshift = bit.rshift
	return string.format("%d.%d.%d.%d",
		band(ip, 0xFF),
		band(rshift(ip, 8), 0xFF),
		band(rshift(ip, 16), 0xFF),
		rshift(ip, 24)
	)
end

function Game.getReverseDirection(direction)
	if direction == WEST then
		return EAST
	elseif direction == EAST then
		return WEST
	elseif direction == NORTH then
		return SOUTH
	elseif direction == SOUTH then
		return NORTH
	elseif direction == NORTHWEST then
		return SOUTHEAST
	elseif direction == NORTHEAST then
		return SOUTHWEST
	elseif direction == SOUTHWEST then
		return NORTHEAST
	elseif direction == SOUTHEAST then
		return NORTHWEST
	end
	return NORTH
end

function Game.getSkillType(weaponType)
	if weaponType == WEAPON_CLUB then
		return SKILL_CLUB
	elseif weaponType == WEAPON_SWORD then
		return SKILL_SWORD
	elseif weaponType == WEAPON_AXE then
		return SKILL_AXE
	elseif weaponType == WEAPON_DISTANCE then
		return SKILL_DISTANCE
	elseif weaponType == WEAPON_SHIELD then
		return SKILL_SHIELD
	end
	return SKILL_FIST
end

if not globalStorageTable then
	globalStorageTable = {}
end

function Game.getStorageValue(key)
	return globalStorageTable[key]
end

function Game.setStorageValue(key, value)
	globalStorageTable[key] = value
end
