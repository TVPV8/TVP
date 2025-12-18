local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and (creature:getPlayer():getVocation():getId() == 4 or creature:getPlayer():getVocation():getId() == 8) then
		doRelocate(item:getPosition(),{x = 32676, y = 32088, z = 09})
	else
		doRelocate(item:getPosition(),{x = 32641, y = 32141, z = 11})
	end
end

moveevent:aid(3090)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 32641, y = 32141, z = 11})
end

moveevent:aid(3090)
moveevent:tileItem(true)
moveevent:register()