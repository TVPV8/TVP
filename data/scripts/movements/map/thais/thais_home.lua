local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() then 
		doRelocate(item:getPosition(),{x = 32369, y = 32241, z = 07})
		creature:getPlayer():setTown(Town("Thais"))
		Game.sendMagicEffect({x = 32369, y = 32241, z = 07}, 13)
	end
end

moveevent:aid(3112)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 32369, y = 32242, z = 06})
	Game.sendMagicEffect({x = 32369, y = 32242, z = 06}, 14)
end

moveevent:aid(3112)
moveevent:tileItem(true)
moveevent:register()