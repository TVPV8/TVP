local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if Game.isItemInPosition({x = 33316, y = 31591, z = 15}, 1387) then
		doRelocate(item:getPosition(),{x = 33328, y = 31592, z = 14})
	end
end

moveevent:aid(3091)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 33316, y = 31591, z = 15}, 1387) then
		doRelocate(item:getPosition(),{x = 33328, y = 31592, z = 14})
	end
end

moveevent:aid(3091)
moveevent:tileItem(true)
moveevent:register()