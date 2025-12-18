local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 33198, y = 32876, z = 11},2335) and Game.isItemInPosition ({x = 33198, y = 32876, z = 11},2336) and Game.isItemInPosition ({x = 33198, y = 32876, z = 11},2337) and Game.isItemInPosition ({x = 33198, y = 32876, z = 11},2338) and Game.isItemInPosition ({x = 33198, y = 32876, z = 11},2339) and Game.isItemInPosition ({x = 33198, y = 32876, z = 11},2340) and Game.isItemInPosition ({x = 33198, y = 32876, z = 11},2341) then 
		Game.removeItemInPosition({x = 33198, y = 32876, z = 11}, 2335)
		Game.removeItemInPosition({x = 33198, y = 32876, z = 11}, 2336)
		Game.removeItemInPosition({x = 33198, y = 32876, z = 11}, 2337)
		Game.removeItemInPosition({x = 33198, y = 32876, z = 11}, 2338)
		Game.removeItemInPosition({x = 33198, y = 32876, z = 11}, 2339)
		Game.removeItemInPosition({x = 33198, y = 32876, z = 11}, 2340)
		Game.removeItemInPosition({x = 33198, y = 32876, z = 11}, 2341)
		Game.createItem(2342, 1, {x = 33198, y = 32876, z = 11})
		Game.sendMagicEffect({x = 33198, y = 32876, z = 11}, 7)
	end
end

moveevent:aid(3018)
moveevent:tileItem(true)
moveevent:register()