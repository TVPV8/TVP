local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() then 
		doRelocate(item:getPosition(),{x = 32649, y = 31925, z = 11})
		creature:getPlayer():setTown(Town("Kazordoon"))
		Game.sendMagicEffect({x = 32649, y = 31925, z = 11}, 13)
	end
end

moveevent:aid(3109)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 32647, y = 31925, z = 12})
	Game.sendMagicEffect({x = 32647, y = 31925, z = 12}, 14)
end

moveevent:aid(3109)
moveevent:tileItem(true)
moveevent:register()