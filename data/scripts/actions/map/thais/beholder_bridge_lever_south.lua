local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
		Game.transformItemInPosition({x = 32413, y = 32230, z = 10}, 1945, 1946)
		doRelocate({x = 32411, y = 32231, z = 10},{x = 32412, y = 32231, z = 10})
		doRelocate({x = 32410, y = 32231, z = 10},{x = 32412, y = 32231, z = 10})
		doRelocate({x = 32411, y = 32232, z = 10},{x = 32412, y = 32232, z = 10})
		doRelocate({x = 32410, y = 32232, z = 10},{x = 32412, y = 32232, z = 10})
		Game.transformItemInPosition({x = 32410, y = 32231, z = 10}, 1284, 493)
		Game.transformItemInPosition({x = 32411, y = 32231, z = 10}, 1284, 493)
		Game.transformItemInPosition({x = 32411, y = 32232, z = 10}, 1284, 493)
		Game.transformItemInPosition({x = 32410, y = 32232, z = 10}, 1284, 493)
		Game.createItem(4799, 1, {x = 32410, y = 32231, z = 10})
		Game.createItem(4799, 1, {x = 32410, y = 32232, z = 10}) 
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		Game.transformItemInPosition({x = 32413, y = 32230, z = 10}, 1946, 1945)
		Game.transformItemInPosition({x = 32411, y = 32231, z = 10}, 493, 1284)
		Game.transformItemInPosition({x = 32411, y = 32232, z = 10}, 493, 1284)
		Game.transformItemInPosition({x = 32410, y = 32231, z = 10}, 493, 1284)
		Game.transformItemInPosition({x = 32410, y = 32232, z = 10}, 493, 1284)
		Game.removeItemInPosition({x = 32410, y = 32231, z = 10}, 4799)
		Game.removeItemInPosition({x = 32410, y = 32232, z = 10}, 4799)
	end
	
	return true
end

action:aid(2060)
action:register()