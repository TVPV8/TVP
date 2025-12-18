local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and creature:getPlayer():getItemCount(2350) >= 1 and creature:getPlayer():getStorageValue(263) == -1 then
		doRelocate(item:getPosition(),{x = 33182, y = 32715, z = 14})
		item:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33182, y = 32715, z = 14}, 11)
		creature:getPlayer():removeItem(2350, 1)
	else
		doRelocate(item:getPosition(),{x = 33231, y = 32705, z = 08})
		item:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33231, y = 32705, z = 08}, 11)
	end
end

moveevent:aid(3069)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 33231, y = 32705, z = 08})
	item:getPosition():sendMagicEffect(11)
	Game.sendMagicEffect({x = 33231, y = 32705, z = 08}, 11)
end

moveevent:aid(3069)
moveevent:tileItem(true)
moveevent:register()