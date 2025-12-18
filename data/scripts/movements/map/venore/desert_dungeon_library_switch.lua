local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() then 
		item:transform(425, 1)
		item:decay()
		Game.removeItemInPosition({x = 32692, y = 32102, z = 10}, 1036)
	end
end

moveevent:aid(3021)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onStepOut(creature, item, position, fromPosition)
	if creature:isPlayer() then 
		item:transform(426, 1)
		item:decay()
		doRelocate({x = 32692, y = 32102, z = 10},{x = 32691, y = 32102, z = 10})
		Game.createItem(1036, 1, {x = 32692, y = 32102, z = 10})
	end
end

moveevent:aid(3021)
moveevent:register()