local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() then
		item:transform(425, 1)
		item:decay()
		Game.removeItemInPosition({x = 32796, y = 31594, z = 05}, 1025)
	end
end

moveevent:aid(3019)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onStepOut(creature, item, position, fromPosition)
	if creature:isPlayer() then 
		item:transform(426, 1)
		item:decay()
		doRelocate({x = 32796, y = 31594, z = 05},{x = 32797, y = 31594, z = 05})
		Game.createItem(1025, 1, {x = 32796, y = 31594, z = 05})
	end
end

moveevent:aid(3019)
moveevent:register()