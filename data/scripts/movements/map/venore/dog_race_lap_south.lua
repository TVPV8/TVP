local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	item:transform(425, 1)
	item:decay()
	Game.transformItemInPosition({x = 32915, y = 32079, z = 5}, 1485, 1484)
end

moveevent:aid(3120)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onStepOut(creature, item, position, fromPosition)
	item:transform(426, 1)
	item:decay()
	Game.transformItemInPosition({x = 32915, y = 32079, z = 5}, 1484, 1485)
end

moveevent:aid(3120)
moveevent:register()