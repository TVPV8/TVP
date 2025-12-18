local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32627, y = 31699, z = 10}, 1284) then
		item:transform(1946, 1)
		item:decay()
		doRelocate({x = 32627, y = 31699, z = 10},{x = 32626, y = 31699, z = 10})
		doRelocate({x = 32628, y = 31699, z = 10},{x = 32626, y = 31699, z = 10})
		doRelocate({x = 32629, y = 31699, z = 10},{x = 32626, y = 31699, z = 10})
		Game.transformItemInPosition({x = 32627, y = 31699, z = 10}, 1284, 493)
		Game.createItem(4799, 1, {x = 32627, y = 31699, z = 10})
		Game.transformItemInPosition({x = 32628, y = 31699, z = 10}, 1284, 493)
		Game.transformItemInPosition({x = 32629, y = 31699, z = 10}, 1284, 493)
		Game.createItem(4797, 1, {x = 32629, y = 31699, z = 10})
	elseif item:getId() == 1945 then
		item:transform(1946, 1)
		item:decay()
	elseif item:getId() == 1946 and Game.isItemInPosition({x = 32628, y = 31699, z = 10}, 493) then
		item:transform(1945, 1)
		item:decay()
		Game.transformItemInPosition({x = 32627, y = 31699, z = 10}, 493, 1284)
		Game.transformItemInPosition({x = 32628, y = 31699, z = 10}, 493, 1284)
		Game.transformItemInPosition({x = 32629, y = 31699, z = 10}, 493, 1284)
		Game.removeItemInPosition({x = 32627, y = 31699, z = 10}, 4799)
		Game.removeItemInPosition({x = 32629, y = 31699, z = 10}, 4797)
	end
	return true
end

action:aid(2001)
action:register()