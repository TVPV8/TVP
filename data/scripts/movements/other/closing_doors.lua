local moveevent = MoveEvent()

function moveevent.onStepOut(creature, item, position, toPosition)
	local tile = Tile(position)
	if tile:getCreatureCount() > 0 then
		return true
	end

	for i = tile:getThingCount() - 1, 0, -1 do
		local tileItem = tile:getThing(i)
		if tileItem and tileItem:getUniqueId() ~= item.uid and tileItem:getType():isMovable() then
			local newPosition = {x = position.x + 1, y = position.y, z = position.z}
			local nexttile = Tile(newPosition)
			local query = nexttile:queryAdd(tileItem)
			if query ~= RETURNVALUE_NOERROR then
				newPosition = {x = position.x, y = position.y + 1, z = position.z}
				nexttile = Tile(newPosition)
				query = nexttile:queryAdd(tileItem)
				if query == RETURNVALUE_NOERROR then
					tileItem:moveTo(newPosition)
				end
			else
				tileItem:moveTo(newPosition)
			end
		end
	end

	local splashItem = tile:getItemByGroup(ITEM_GROUP_SPLASH)
	if splashItem then
		splashItem:remove()
	end
	
	local magicField = tile:getItemByGroup(ITEM_GROUP_MAGICFIELD)
	if magicField then
		magicField:remove()
	end

	item:transform(item.itemid - 1)
	return true
end

for _, id in pairs(openLevelDoors) do
	moveevent:id(id)
end
for _, id in pairs(openQuestDoors) do
	moveevent:id(id)
end

moveevent:register()