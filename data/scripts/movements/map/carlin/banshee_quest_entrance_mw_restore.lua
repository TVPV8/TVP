local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if not Game.isItemInPosition({x = 32266, y = 31861, z = 11}, 1945) then
		Game.transformItemInPosition({x = 32266, y = 31861, z = 11}, 1946, 1945)
		Game.transformItemInPosition({x = 32266, y = 31860, z = 11}, 408, 407)
		Game.createItem(1498, 1, {x = 32266, y = 31860, z = 11})
	end
end

moveevent:aid(3024)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if not Game.isItemInPosition({x = 32266, y = 31861, z = 11}, 1945) then
		Game.transformItemInPosition({x = 32266, y = 31861, z = 11}, 1946, 1945)
		Game.transformItemInPosition({x = 32266, y = 31860, z = 11}, 408, 407)
		Game.createItem(1498, 1, {x = 32266, y = 31860, z = 11})
	end
end

moveevent:aid(3024)
moveevent:tileItem(true)
moveevent:register()