local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and creature:getPlayer():getItemCount(2352) >= 1 and creature:getPlayer():getStorageValue(267) == -1 then
		doRelocate(item:getPosition(),{x = 33178, y = 33016, z = 14})
		item:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33178, y = 33016, z = 14}, 11)
		creature:getPlayer():removeItem(2352, 1)
	else
		doRelocate(item:getPosition(),{x = 33025, y = 32872, z = 08})
		item:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33025, y = 32872, z = 08}, 11)
	end
end

moveevent:aid(3066)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 33025, y = 32872, z = 08})
	item:getPosition():sendMagicEffect(11)
	Game.sendMagicEffect({x = 33025, y = 32872, z = 08}, 11)
end

moveevent:aid(3066)
moveevent:tileItem(true)
moveevent:register()