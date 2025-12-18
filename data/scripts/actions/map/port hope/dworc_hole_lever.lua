local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		Game.removeItemInPosition({x = 32649, y = 32923, z = 08}, 1335)
		Game.transformItemInPosition({x = 32649, y = 32923, z = 08}, 351, 383)
		Game.transformItemInPosition({x = 32652, y = 32922, z = 08}, 1945, 1946)
	elseif item:getId() == 1946 then 
		Game.transformItemInPosition({x = 32649, y = 32923, z = 08}, 383, 351)
		Game.createItem(1335, 1, {x = 32649, y = 32923, z = 08})
		Game.transformItemInPosition({x = 32652, y = 32922, z = 08}, 1946, 1945)
	end
	return true
end

action:aid(2042)
action:register()