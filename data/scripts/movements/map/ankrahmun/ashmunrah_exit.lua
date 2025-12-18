local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if Game.isItemInPosition({x = 33176, y = 32880, z = 11},1946) and Game.isItemInPosition ({x = 33175, y = 32884, z = 11},1946) and Game.isItemInPosition ({x = 33176, y = 32889, z = 11},1946) and Game.isItemInPosition ({x = 33182, y = 32880, z = 11},1946) and Game.isItemInPosition ({x = 33183, y = 32884, z = 11},1946) and Game.isItemInPosition ({x = 33181, y = 32889, z = 11}, 1946) then
		doRelocate(item:getPosition(),{x = 33198, y = 32885, z = 11})
		item:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33198, y = 32885, z = 11}, 11)
		Game.transformItemInPosition({x = 33176, y = 32880, z = 11}, 1946, 1945)
		Game.transformItemInPosition({x = 33175, y = 32884, z = 11}, 1946, 1945)
		Game.transformItemInPosition({x = 33176, y = 32889, z = 11}, 1946, 1945)
		Game.transformItemInPosition({x = 33182, y = 32880, z = 11}, 1946, 1945)
		Game.transformItemInPosition({x = 33183, y = 32884, z = 11}, 1946, 1945)
		Game.transformItemInPosition({x = 33181, y = 32889, z = 11}, 1946, 1945)
	else
		doRelocate(item:getPosition(),{x = 33179, y = 32889, z = 11})
		item:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33179, y = 32889, z = 11}, 11)
	end
end

moveevent:aid(3068)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 33179, y = 32889, z = 11})
	item:getPosition():sendMagicEffect(11)
	Game.sendMagicEffect({x = 33179, y = 32889, z = 11}, 11)
end

moveevent:aid(3068)
moveevent:tileItem(true)
moveevent:register()