local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 33293, y = 32741, z = 13},2159) then 
		doRelocate({x = 33293, y = 32742, z = 13},{x = 33299, y = 32742, z = 13})
		Game.sendMagicEffect({x = 33293, y = 32742, z = 13}, 11)
		Game.sendMagicEffect({x = 33299, y = 32742, z = 13}, 11)
		Game.sendMagicEffect({x = 33293, y = 32741, z = 13}, 16)
		Game.removeItemInPosition({x = 33293, y = 32741, z = 13}, 2159)
	else 
		Game.sendMagicEffect({x = 33293, y = 32741, z = 13}, 3)
	end
end

moveevent:aid(3053)
moveevent:tileItem(true)
moveevent:register()