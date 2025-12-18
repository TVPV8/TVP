local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and creature:getPlayer():getItemCount(2354) >= 1 and creature:getPlayer():getStorageValue(265) == -1 then
		doRelocate(item:getPosition(),{x = 33126, y = 32592, z = 15})
		item:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33126, y = 32591, z = 15}, 11)
		creature:getPlayer():removeItem(2354, 1)
	else
		doRelocate(item:getPosition(),{x = 33131, y = 32566, z = 08})
		item:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33131, y = 32566, z = 08}, 11)
	end
end

moveevent:aid(3072)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 33131, y = 32566, z = 08})
	item:getPosition():sendMagicEffect(11)
	Game.sendMagicEffect({x = 33131, y = 32566, z = 08}, 11)
end

moveevent:aid(3072)
moveevent:tileItem(true)
moveevent:register()