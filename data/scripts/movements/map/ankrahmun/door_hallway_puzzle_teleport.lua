local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:getPlayer():getStorageValue(260) == 1 then
		doRelocate(item:getPosition(),{x = 33095, y = 32590, z = 15})
		item:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33095, y = 32590, z = 15}, 11)
		creature:getPlayer():setStorageValue(260, 0)
	else
		doRelocate(item:getPosition(),{x = 33073, y = 32604, z = 15})
		item:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33073, y = 32604, z = 15}, 11)
	end
end

moveevent:aid(3075)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 33073, y = 32604, z = 15})
	item:getPosition():sendMagicEffect(11)
	Game.sendMagicEffect({x = 33073, y = 32604, z = 15}, 11)
end

moveevent:aid(3075)
moveevent:tileItem(true)
moveevent:register()