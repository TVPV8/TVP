local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 33233, y = 32692, z = 13},2159) then 
		doRelocate({x = 33234, y = 32692, z = 13},{x = 33234, y = 32687, z = 13})
		Game.sendMagicEffect({x = 33234, y = 32692, z = 13}, 11)
		Game.sendMagicEffect({x = 33234, y = 32687, z = 13}, 11)
		Game.sendMagicEffect({x = 33233, y = 32692, z = 13}, 16)
		Game.removeItemInPosition({x = 33233, y = 32692, z = 13}, 2159)
	else
		Game.sendMagicEffect({x = 33233, y = 32692, z = 13}, 3)
	end
end

moveevent:aid(3056)
moveevent:tileItem(true)
moveevent:register()