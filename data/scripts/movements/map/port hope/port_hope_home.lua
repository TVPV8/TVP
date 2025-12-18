local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() then 
		doRelocate(item:getPosition(),{x = 32595, y = 32744, z = 06})
		creature:getPlayer():setTown(Town("Port Hope"))
		Game.sendMagicEffect({x = 32595, y = 32744, z = 06}, 13)
	end
end

moveevent:aid(3111)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 32595, y = 32744, z = 06})
	Game.sendMagicEffect({x = 32595, y = 32744, z = 06}, 14)
end

moveevent:aid(3111)
moveevent:tileItem(true)
moveevent:register()