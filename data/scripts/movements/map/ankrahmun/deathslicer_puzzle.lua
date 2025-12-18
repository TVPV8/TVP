local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:getPlayer():getStorageValue(271) > 0 and creature:getPlayer():getStorageValue(272) > 0 and creature:getPlayer():getStorageValue(273) > 0 and creature:getPlayer():getStorageValue(274) > 0 and creature:getPlayer():getStorageValue(275) > 0 and creature:getPlayer():getStorageValue(276) > 0 and creature:getPlayer():getStorageValue(277) > 0 then
		doRelocate(item:getPosition(),{x = 33164, y = 32694, z = 14})
		item:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33164, y = 32694, z = 14}, 11)
		creature:getPlayer():setStorageValue(271,0)
		creature:getPlayer():setStorageValue(272,0)
		creature:getPlayer():setStorageValue(273,0)
		creature:getPlayer():setStorageValue(274,0)
		creature:getPlayer():setStorageValue(275,0)
		creature:getPlayer():setStorageValue(276,0)
		creature:getPlayer():setStorageValue(277,0)
	else
		doRelocate(item:getPosition(),{x = 33259, y = 32707, z = 13})
		Game.sendMagicEffect({x = 33259, y = 32707, z = 13}, 11)
	end
end

moveevent:aid(3065)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(tileitem:getPosition(),{x = 33259, y = 32707, z = 13})
	Game.sendMagicEffect({x = 33259, y = 32707, z = 13}, 11)
end

moveevent:aid(3065)
moveevent:tileItem(true)
moveevent:register()