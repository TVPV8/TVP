local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and creature:getPlayer():getItemCount(2353) >= 1 and creature:getPlayer():getStorageValue(261) == -1 then
		doRelocate(item:getPosition(),{x = 33174, y = 32937, z = 15})
		item:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33174, y = 32937, z = 15}, 11)
		creature:getPlayer():removeItem(2353, 1)
	else
		doRelocate(item:getPosition(),{x = 33255, y = 32836, z = 08})
		item:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33255, y = 32836, z = 08}, 11)
	end
end

moveevent:aid(3067)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 33255, y = 32836, z = 08})
	item:getPosition():sendMagicEffect(11)
	Game.sendMagicEffect({x = 33255, y = 32836, z = 08}, 11)
end

moveevent:aid(3067)
moveevent:tileItem(true)
moveevent:register()