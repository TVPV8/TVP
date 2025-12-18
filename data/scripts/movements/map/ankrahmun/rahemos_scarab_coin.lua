local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 33135, y = 32682, z = 12},2159) then 
		doRelocate({x = 33135, y = 32683, z = 12},{x = 33130, y = 32683, z = 12})
		Game.sendMagicEffect({x = 33135, y = 32683, z = 12}, 11)
		Game.sendMagicEffect({x = 33130, y = 32683, z = 12}, 11)
		Game.sendMagicEffect({x = 33135, y = 32682, z = 12}, 16)
		Game.removeItemInPosition({x = 33135, y = 32682, z = 12}, 2159)
	else
		Game.sendMagicEffect({x = 33135, y = 32682, z = 12}, 3)
	end
end

moveevent:aid(3058)
moveevent:tileItem(true)
moveevent:register()