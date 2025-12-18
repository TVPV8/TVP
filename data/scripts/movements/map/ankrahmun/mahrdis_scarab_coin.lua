local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 33240, y = 32855, z = 13},2159) then 
		doRelocate({x = 33240, y = 32856, z = 13},{x = 33246, y = 32850, z = 13})
		Game.sendMagicEffect({x = 33240, y = 32856, z = 13}, 11)
		Game.sendMagicEffect({x = 33246, y = 32850, z = 13}, 11)
		Game.sendMagicEffect({x = 33240, y = 32855, z = 13}, 16)
		Game.removeItemInPosition({x = 33240, y = 32855, z = 13}, 2159)
	else
		Game.sendMagicEffect({x = 33240, y = 32855, z = 13}, 3)
	end
end

moveevent:aid(3055)
moveevent:tileItem(true)
moveevent:register()