local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	doRelocate(item:getPosition(),{x = 33225, y = 32699, z = 14})
	item:getPosition():sendMagicEffect(11)
	Game.sendMagicEffect({x = 33225, y = 32699, z = 14}, 11)
end

moveevent:aid(3017)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(tileitem:getPosition(),{x = 33225, y = 32699, z = 14})
	tileitem:getPosition():sendMagicEffect(11)
	Game.sendMagicEffect({x = 33225, y = 32699, z = 14}, 11)
end

moveevent:aid(3017)
moveevent:tileItem(true)
moveevent:register()