local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() then 
		doRelocate(item:getPosition(),{x = 33213, y = 32454, z = 01})
		creature:getPlayer():setTown(Town("Darashia"))
		Game.sendMagicEffect({x = 33213, y = 32454, z = 01}, 13)
	end
end

moveevent:aid(3105)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 33212, y = 32455, z = 02})
	Game.sendMagicEffect({x = 33212, y = 32455, z = 02}, 14)
end

moveevent:aid(3105)
moveevent:tileItem(true)
moveevent:register()