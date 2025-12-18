local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 33098, y = 32816, z = 13},2159) then
		doRelocate({x = 33097, y = 32816, z = 13},{x = 33093, y = 32824, z = 13})
		Game.sendMagicEffect({x = 33097, y = 32816, z = 13}, 11)
		Game.sendMagicEffect({x = 33093, y = 32824, z = 13}, 11)
		Game.sendMagicEffect({x = 33098, y = 32816, z = 13}, 16)
		Game.removeItemInPosition({x = 33098, y = 32816, z = 13}, 2159)
	else
		Game.sendMagicEffect({x = 33098, y = 32816, z = 13}, 3)
	end
end

moveevent:aid(3059)
moveevent:tileItem(true)
moveevent:register()