local steppingIn = {[416] = 417, [426] = 425, [446] = 447, [3216] = 3217, [3202] = 3215}
local steppingOut = {[417] = 416, [425] = 426, [447] = 446, [3217] = 3216, [3215] = 3202}

local moveeventStepIn = MoveEvent()
local moveeventStepOut = MoveEvent()

function moveeventStepIn.onStepIn(creature, item, position, fromPosition)
	if not steppingIn[item.itemid] then
		return true
	end

	if not creature:isPlayer() or creature:isInGhostMode() then
		return true
	end

	if Tile(position):hasFlag(TILESTATE_PROTECTIONZONE) then
		for x = -1, 1 do
			for y = -1, 1 do
				local pos = {x = position.x, y = position.y, z = position.z}
				pos.x = pos.x + x
				pos.y = pos.y + y
				local depotItem = Tile(pos):getItemByType(ITEM_TYPE_DEPOT)
				if depotItem then
					creature:loadDepotLocker(getDepotId(depotItem:getUniqueId()))
					local depotItems = math.max(1, creature:getDepotLocker(getDepotId(depotItem:getUniqueId())):getItemHoldingCount())
					if position.x ~= fromPosition.x or position.y ~= fromPosition.y or position.z ~= fromPosition. z then
						creature:sendTextMessage(MESSAGE_STATUS_DEFAULT, "Your depot contains " .. depotItems .. " item" .. (depotItems > 1 and "s." or "."))
						if depotItems > creature:getMaxDepotItems() then
							creature:sendTextMessage(MESSAGE_INFO_DESCR, "Your depot is full. Remove surplus items before storing new ones.")
						end
					end
					item:transform(steppingIn[item.itemid])
					return true
				end
			end
		end
	end

	if item:getActionId() ~= 0 then
		-- tiles with action do not transform if there is no code for them to
		return true
	end

	item:transform(steppingIn[item.itemid])
	return true
end

function moveeventStepOut.onStepOut(creature, item, position, fromPosition)
	if not steppingOut[item.itemid] then
		return true
	end

	if creature:isPlayer() and creature:isInGhostMode() then
		return true
	end

	for x = -1, 1 do
		for y = -1, 1 do
			local pos = {x = position.x, y = position.y, z = position.z}
			pos.x = pos.x + x
			pos.y = pos.y + y
			local depotItem = Tile(pos):getItemByType(ITEM_TYPE_DEPOT)
			if depotItem then
				creature:unloadDepotLocker(getDepotId(depotItem:getUniqueId()))
				break
			end
		end
	end

	if not Tile(position):hasFlag(TILESTATE_PROTECTIONZONE) and item:getActionId() ~= 0 then
		-- tiles with no action should not transform
		return true
	end

	if Tile(item:getPosition()):getCreatureCount() == 0 then
		item:transform(steppingOut[item.itemid])
	end
	return true
end

for id in pairs(steppingIn) do
	moveeventStepIn:id(id)
end

for id in pairs(steppingOut) do
	moveeventStepOut:id(id)
end

moveeventStepIn:register()
moveeventStepOut:register()