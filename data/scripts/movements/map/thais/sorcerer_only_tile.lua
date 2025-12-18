local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and (creature:getPlayer():getVocation():getId() == 1 or creature:getPlayer():getVocation():getId() == 5) then 
		item:transform(447, 1)
		item:decay()
	else
		creature:teleportTo({x = 32308, y = 32267, z = 7})
	end
end

moveevent:aid(3117)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onStepOut(creature, item, position, fromPosition)
	item:transform(446, 1)
	item:decay()
end

moveevent:aid(3117)
moveevent:register()