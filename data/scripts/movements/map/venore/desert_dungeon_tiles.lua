local vocationTiles = {
	{pos = {x = 32673, y = 32085, z = 8}, vocations = {3, 7}},
	{pos = {x = 32677, y = 32089, z = 8}, vocations = {1, 5}},
	{pos = {x = 32669, y = 32089, z = 8}, vocations = {2, 6}},
	{pos = {x = 32673, y = 32093, z = 8}, vocations = {4, 8}},
}

local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	local player = Player(creature)
	if not player then
		return false
	end
	
	for _, data in ipairs(vocationTiles) do
		if position.x == data.pos.x and position.y == data.pos.y and position.z == data.pos.z then
			if table.contains(data.vocations, player:getVocation():getId()) then
				item:transform(425)
				break
			end
		end
	end

	return false
end

moveevent:aid(3000)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onStepOut(creature, item, position, fromPosition)
	item = Tile(position):getItemById(425)
	if item then
		item:transform(426)
	end
	return false
end

moveevent:aid(3000)
moveevent:register()