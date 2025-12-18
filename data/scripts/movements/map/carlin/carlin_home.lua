local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() then 
		doRelocate(item:getPosition(),{x = 32360, y = 31782, z = 07})
		creature:getPlayer():setTown(Town("Carlin"))
		Game.sendMagicEffect({x = 32360, y = 31782, z = 07}, 13)
	end
end

moveevent:aid(3113)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 32360, y = 31780, z = 08})
	Game.sendMagicEffect({x = 32360, y = 31780, z = 08}, 14)
end

moveevent:aid(3113)
moveevent:tileItem(true)
moveevent:register()