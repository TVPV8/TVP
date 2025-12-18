local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and (creature:getPlayer():getVocation():getId() == 2 or creature:getPlayer():getVocation():getId() == 6) then
		doRelocate(item:getPosition(),{x = 32851, y = 32339, z = 06})
	else
		doRelocate(item:getPosition(),{x = 32836, y = 32294, z = 07}) 
	end
end

moveevent:aid(3116)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 32836, y = 32294, z = 07}) 
end

moveevent:aid(3116)
moveevent:tileItem(true)
moveevent:register()