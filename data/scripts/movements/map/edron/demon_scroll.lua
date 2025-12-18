local moveevent = MoveEvent()

function moveevent.onRemoveItem(item, tileitem, position)
	if Game.getStorageValue(GlobalStorageKeys.edronDemonScroll) == 1 then
		return
	end
	
	Game.createMonster("Demon", {x = 33060, y = 31623, z = 15}, true)
	Game.createMonster("Demon", {x = 33066, y = 31623, z = 15}, true)
	Game.createMonster("Demon", {x = 33066, y = 31627, z = 15}, true)
	Game.createMonster("Demon", {x = 33060, y = 31627, z = 15}, true)

	Game.sendMagicEffect({x = 33060, y = 31622, z = 15}, 14)
	Game.sendMagicEffect({x = 33066, y = 31622, z = 15}, 14)
	Game.sendMagicEffect({x = 33066, y = 31628, z = 15}, 14)
	Game.sendMagicEffect({x = 33060, y = 31628, z = 15}, 14)
	
	Game.setStorageValue(GlobalStorageKeys.edronDemonScroll, 1)
end

moveevent:aid(3121)
moveevent:tileItem(true)
moveevent:register()