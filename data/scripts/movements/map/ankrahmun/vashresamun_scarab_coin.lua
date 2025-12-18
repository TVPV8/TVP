local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 33276, y = 32552, z = 14},2159) then 
		doRelocate({x = 33276, y = 32553, z = 14},{x = 33271, y = 32553, z = 14})
		Game.sendMagicEffect({x = 33276, y = 32553, z = 14}, 11)
		Game.sendMagicEffect({x = 33271, y = 32553, z = 14}, 11)
		Game.sendMagicEffect({x = 33276, y = 32552, z = 14}, 16)
		Game.removeItemInPosition({x = 33276, y = 32552, z = 14}, 2159)
	else
		Game.sendMagicEffect({x = 33276, y = 32552, z = 14}, 3)
	end
end

moveevent:aid(3054)
moveevent:tileItem(true)
moveevent:register()