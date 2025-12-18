local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if Game.isItemInPosition({x = 32803, y = 31584, z = 01},1946) and Game.isItemInPosition ({x = 32805, y = 31584, z = 01},1946) and Game.isItemInPosition ({x = 32802, y = 31584, z = 01},1945) and Game.isItemInPosition ({x = 32804, y = 31584, z = 01}, 1945) then 
		doRelocate(item:getPosition(),{x = 32701, y = 31639, z = 06})
		Game.sendMagicEffect({x = 32701, y = 31639, z = 06}, 11)
	else
		doRelocate(item:getPosition(),{x = 32803, y = 31587, z = 01})
		Game.sendMagicEffect({x = 32803, y = 31587, z = 01}, 11)
	end
end

moveevent:aid(3104)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 32803, y = 31587, z = 01})
	Game.sendMagicEffect({x = 32803, y = 31587, z = 01}, 11)
end

moveevent:aid(3104)
moveevent:tileItem(true)
moveevent:register()