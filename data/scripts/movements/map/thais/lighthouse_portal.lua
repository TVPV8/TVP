local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if Game.isItemInPosition({x = 32233, y = 32276, z = 9}, 1387) then
		doRelocate(item:getPosition(),{x = 32225, y = 32275, z = 10})
		Game.sendMagicEffect({x = 32233, y = 32276, z = 09}, 15)
		Game.sendMagicEffect({x = 32225, y = 32275, z = 10}, 15)
	end
end

moveevent:aid(3114)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 32233, y = 32276, z = 9}, 1387) then
		doRelocate(item:getPosition(),{x = 32225, y = 32275, z = 10})
		Game.sendMagicEffect({x = 32233, y = 32276, z = 09}, 15)
		Game.sendMagicEffect({x = 32225, y = 32275, z = 10}, 15)
	end
end

moveevent:aid(3114)
moveevent:tileItem(true)
moveevent:register()