Combat.radiusArea = {
	{0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 8, 8, 7, 8, 8, 0, 0, 0, 0},
	{0, 0, 0, 8, 7, 6, 6, 6, 7, 8, 0, 0, 0},
	{0, 0, 8, 7, 6, 5, 5, 5, 6, 7, 8, 0, 0},
	{0, 8, 7, 6, 5, 4, 4, 4, 5, 6, 7, 8, 0},
	{0, 8, 6, 5, 4, 3, 2, 3, 4, 5, 6, 8, 0},
	{8, 7, 6, 5, 4, 2, 1, 2, 4, 5, 6, 7, 8},
	{0, 8, 6, 5, 4, 3, 2, 3, 4, 5, 6, 8, 0},
	{0, 8, 7, 6, 5, 4, 4, 4, 5, 6, 7, 8, 0},
	{0, 0, 8, 7, 6, 5, 5, 5, 6, 7, 8, 0, 0},
	{0, 0, 0, 8, 7, 6, 6, 6, 7, 8, 0, 0, 0},
	{0, 0, 0, 0, 8, 8, 7, 8, 8, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0}
}

function assembleAreaExecution(centerPos, dir, area, checkSightOfLine)
	local hitTiles = {}
	local areaWidth = #area[1]
	local areaHeight = table.getn(area)

	local centerX = (areaWidth - 1) / 2
	local centerY = (areaHeight - 1) / 2
	
	-- Go through the area array, and assemble all tiles that match the direction
	for rowIndex, rows in pairs(area) do
		for colIndex, value in ipairs(rows) do
			if		(value:find("a") or value:find("%[a%]") ) or
					(dir == NORTHWEST and ( value:find("%[nw%]") or value:find("%[wn%]")) ) or
					(dir == NORTHEAST and ( value:find("%[ne%]") or value:find("%[en%]")) ) or
					(dir == SOUTHWEST and ( value:find("%[sw%]") or value:find("%[ws%]")) ) or
					(dir == SOUTHEAST and ( value:find("%[se%]") or value:find("%[es%]")) ) or
					(dir == NORTH     and ( value == "n" or string.find(value, "%[n%]")) ) or
					(dir == SOUTH     and ( value == "s" or string.find(value, "%[s%]")) ) or
					(dir == WEST      and ( value == "w" or string.find(value, "%[w%]")) ) or
					(dir == EAST      and ( value == "e" or value:find("%[e%]")) )
					then
				local posx = centerPos.x + (centerX - (areaWidth - 1)) + colIndex - 1
				local posy = centerPos.y + (centerY - (areaHeight - 1)) + rowIndex - 1

				local pos = {x = posx, y = posy, z = centerPos.z}

				if not checkSightOfLine or centerPos:isSightClear(pos, false) then
					local tile = Tile(pos)
					hitTiles[pos] = tile
				end
			end
		end
	end
	return hitTiles
end

function Combat:getCircleArea(caster, radius, checkSightOfLine)
	local area = {}	
	
	for y, rows in pairs(Combat.radiusArea) do
		area[y] = {}
		for x, value in ipairs(rows) do
			if value > 0 and value <= radius then
				table.insert(area[y], "a")
			else
				table.insert(area[y], " ")
			end
		end
	end

	return assembleAreaExecution(caster:getPosition(), caster:getDirection(), area, checkSightOfLine)
end

function Combat:getLineArea(caster, length, spread)
	local area = {}	
	
	local areaWidth = length * 2
	if areaWidth % 2 == 0 then
		areaWidth = areaWidth + 1
	end

	local areaHeight = length * 2
	if areaHeight % 2 == 0 then
		areaHeight = areaHeight + 1
	end
	
	local centerX = (areaWidth - 1) / 2 + 1
	local centerY = (areaHeight - 1) / 2 + 1
	
	local cols = 1
	if spread ~= 0 then
		cols = ((length - length % spread) / spread) * 2 + 1
	end
	
	for y = 1,areaHeight do
		area[y] = {}
		for x = 1,areaWidth do
			table.insert(area[y], " ")
		end
	end

	local colSpread = (cols - 1) / 2
	for y = 1, areaHeight do
		
		if y < centerY then
			--build north area
			for x = centerX - colSpread, centerX + colSpread do
				area[y][x] = "n"
			end

			if spread > 0 and y % spread == 0 then
				colSpread = colSpread - 1
			end
		elseif y > centerY then
			--build south area
			if spread > 0 and y % spread == 0 then
				colSpread = colSpread + 1
			end

			for x = centerX - colSpread, centerX + colSpread do
				area[y][x] = "s"
			end
		end
	end	

	local rowSpread = (cols - 1) / 2
	for x = 1, areaWidth  do
		
		if x < centerX then
			--build west area
			for y = centerY - rowSpread, centerY + rowSpread do
				area[y][x] = "w"
			end

			if spread > 0 and x % spread == 0 then
				rowSpread = rowSpread - 1
			end
		elseif x > centerX then
			if spread > 0 and x % spread == 0 then
				rowSpread = rowSpread + 1
			end
			
			--build east area
			for y = centerY - rowSpread, centerY + rowSpread do
				area[y][x] = "e"
			end
		end
	end
	
	area[centerY][centerX] = " "
	
	--[[
	local line = ""
	for y, rows in pairs(area) do
		area[y] = {}		
		
		for x, value in ipairs(rows) do
			line = line .. value
		end

		print(line)
		line = ""
	end
	--]]
	
	return assembleAreaExecution(caster:getPosition(), caster:getDirection(), area, checkSightOfLine)
end

function Combat:getPositions(creature, variant)
	local positions = {}
	function onTargetTile(creature, position)
		positions[#positions + 1] = position
	end

	self:setCallback(CALLBACK_PARAM_TARGETTILE, "onTargetTile")
	self:execute(creature, variant)
	return positions
end

function Combat:getTargets(creature, variant)
	local targets = {}
	function onTargetCreature(creature, target)
		targets[#targets + 1] = target
	end

	self:setCallback(CALLBACK_PARAM_TARGETCREATURE, "onTargetCreature")
	self:execute(creature, variant)
	return targets
end
