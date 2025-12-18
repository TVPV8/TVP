local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
		Game.transformItemInPosition({x = 32104, y = 32204, z = 08}, 1945, 1946)
		Game.transformItemInPosition({x = 32100, y = 32205, z = 08}, 493, 1284)
		Game.transformItemInPosition({x = 32101, y = 32205, z = 08}, 493, 1284)
		Game.removeItemInPosition({x = 32100, y = 32205, z = 08}, 4799)
		Game.removeItemInPosition({x = 32101, y = 32205, z = 08}, 4797)
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		Game.transformItemInPosition({x = 32104, y = 32204, z = 08}, 1946, 1945)
		doRelocate({x = 32100, y = 32205, z = 08},{x = 32102, y = 32205, z = 08})
		doRelocate({x = 32101, y = 32205, z = 08},{x = 32102, y = 32205, z = 08})
		Game.transformItemInPosition({x = 32100, y = 32205, z = 08}, 1284, 493)
		Game.transformItemInPosition({x = 32101, y = 32205, z = 08}, 1284, 493)
		Game.createItem(4799, 1, {x = 32100, y = 32205, z = 08})
		Game.createItem(4797, 1, {x = 32101, y = 32205, z = 08})
	end

	return true
end

action:aid(2054)
action:register()