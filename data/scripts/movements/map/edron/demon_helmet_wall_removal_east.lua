local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and Game.isItemInPosition({x = 33210, y = 31630, z = 13},1050) then 
		local tile = Tile({x = 33190, y = 31629, z = 13})
		if tile:getTopCreature() and tile:getTopCreature():getPlayer() then
			Game.removeItemInPosition({x = 33210, y = 31630, z = 13}, 1050)
			Game.removeItemInPosition({x = 33211, y = 31630, z = 13}, 1050)
			Game.removeItemInPosition({x = 33212, y = 31630, z = 13}, 1050)
		end
	end
	return true
end

moveevent:aid(3022)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onStepOut(creature, item, position, fromPosition)
	if creature:isPlayer() then 
		if not Game.isItemInPosition({x = 33210, y = 31630, z = 13},1050) then
			doRelocate({x = 33210, y = 31630, z = 13},{x = 33210, y = 31631, z = 13})
			doRelocate({x = 33211, y = 31630, z = 13},{x = 33211, y = 31631, z = 13})
			doRelocate({x = 33212, y = 31630, z = 13},{x = 33212, y = 31631, z = 13})
			Game.createItem(1050, 1, {x = 33210, y = 31630, z = 13})
			Game.createItem(1050, 1, {x = 33211, y = 31630, z = 13})
			Game.createItem(1050, 1, {x = 33212, y = 31630, z = 13})
		end
	end
	return true
end

moveevent:aid(3022)
moveevent:register()