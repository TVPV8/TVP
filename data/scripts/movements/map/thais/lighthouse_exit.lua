local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if Game.isItemInPosition({x = 32225, y = 32276, z = 10}, 1387) then
		doRelocate(item:getPosition(),{x = 32232, y = 32276, z = 09})
		item:getPosition():sendMagicEffect(15)
		Game.sendMagicEffect({x = 32232, y = 32276, z = 09}, 15)
	end
end

moveevent:aid(3115)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 32225, y = 32276, z = 10}, 1387) then
		doRelocate(item:getPosition(),{x = 32232, y = 32276, z = 09})
		item:getPosition():sendMagicEffect(15)
		Game.sendMagicEffect({x = 32232, y = 32276, z = 09}, 15)
	end
end

moveevent:aid(3115)
moveevent:tileItem(true)
moveevent:register()