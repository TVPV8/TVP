local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32090, y = 32148, z = 09},1945) and Game.isItemInPosition ({x = 32092, y = 32148, z = 09},1945) and Game.isItemInPosition ({x = 32094, y = 32148, z = 09},1945) and Game.isItemInPosition ({x = 32088, y = 32148, z = 09},1945) then
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 32090, y = 32149, z = 10}, 1037)
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 32090, y = 32149, z = 10},{x = 32090, y = 32150, z = 10})
		Game.createItem(1037, 1, {x = 32090, y = 32149, z = 10})
	end

	return true
end

action:aid(2057)
action:register()