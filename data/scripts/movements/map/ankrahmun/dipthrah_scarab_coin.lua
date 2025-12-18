local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 33073, y = 32589, z = 13},2159) then 
		doRelocate({x = 33073, y = 32590, z = 13},{x = 33080, y = 32588, z = 13})
		Game.sendMagicEffect({x = 33073, y = 32590, z = 13}, 11)
		Game.sendMagicEffect({x = 33080, y = 32589, z = 13}, 11)
		Game.sendMagicEffect({x = 33073, y = 32589, z = 13}, 16)
		Game.removeItemInPosition({x = 33073, y = 32589, z = 13}, 2159)
	else
		Game.sendMagicEffect({x = 33073, y = 32589, z = 13}, 3)
	end
end

moveevent:aid(3060)
moveevent:tileItem(true)
moveevent:register()