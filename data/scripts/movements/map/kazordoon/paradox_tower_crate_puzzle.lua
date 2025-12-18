local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 32476, y = 31900, z = 05},1739) and not Game.isItemInPosition ({x = 32478, y = 31904, z = 05}, 1386) then 
		Game.createItem(1386, 1, {x = 32478, y = 31904, z = 05})
		Game.transformItemInPosition({x = 32476, y = 31900, z = 05}, 426, 425)
	elseif Game.isItemInPosition({x = 32476, y = 31900, z = 05}, 1739) then
		Game.transformItemInPosition({x = 32476, y = 31900, z = 05}, 426, 425)
	end
end

moveevent:aid(3048)
moveevent:tileItem(true)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onRemoveItem(item, tileitem, position)
	if Game.isItemInPosition({x = 32478, y = 31904, z = 05}, 1386) then 
		Game.removeItemInPosition({x = 32478, y = 31904, z = 05}, 1386)
		Game.transformItemInPosition({x = 32476, y = 31900, z = 05}, 425, 426)
	else
		Game.transformItemInPosition({x = 32476, y = 31900, z = 05}, 425, 426)
	end
end

moveevent:aid(3048)
moveevent:tileItem(true)
moveevent:register()