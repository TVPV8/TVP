local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 33161, y = 32831, z = 10},2159) then 
		doRelocate({x = 33162, y = 32831, z = 10},{x = 33158, y = 32832, z = 10})
		Game.sendMagicEffect({x = 33162, y = 32831, z = 10}, 11)
		Game.sendMagicEffect({x = 33158, y = 32832, z = 10}, 11)
		Game.sendMagicEffect({x = 33161, y = 32831, z = 10}, 16)
		Game.removeItemInPosition({x = 33161, y = 32831, z = 10}, 2159)
	else
		Game.sendMagicEffect({x = 33161, y = 32831, z = 10}, 3)
	end
end

moveevent:aid(3057)
moveevent:tileItem(true)
moveevent:register()