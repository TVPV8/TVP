local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 33148, y = 32867, z = 09}, 1498) and Game.isItemInPosition ({x = 33149, y = 32867, z = 09}, 1498) and Game.isItemInPosition ({x = 33148, y = 32868, z = 09}, 1498) and Game.isItemInPosition ({x = 33149, y = 32868, z = 09}, 1498) then 
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 33148, y = 32867, z = 09}, 1498)
		Game.removeItemInPosition({x = 33149, y = 32867, z = 09}, 1498)
		Game.removeItemInPosition({x = 33148, y = 32868, z = 09}, 1498)
		Game.removeItemInPosition({x = 33149, y = 32868, z = 09}, 1498)
	elseif item:getId() == 1945 then
		item:transform(1946, 1)
		item:decay()
	elseif item:getId() == 1946 and Game.isItemInPosition({x = 33148, y = 32867, z = 09}, 1498) then 
		item:transform(1945, 1)
		item:decay()
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 33148, y = 32867, z = 09}, {x = 33148, y = 32869, z = 09})
		doRelocate({x = 33149, y = 32867, z = 09}, {x = 33149, y = 32869, z = 09})
		doRelocate({x = 33148, y = 32868, z = 09}, {x = 33148, y = 32869, z = 09})
		doRelocate({x = 33149, y = 32868, z = 09},{x = 33149, y = 32869, z = 09})
		Game.createItem(1498, 1, {x = 33148, y = 32867, z = 09})
		Game.createItem(1498, 1, {x = 33149, y = 32867, z = 09})
		Game.createItem(1498, 1, {x = 33148, y = 32868, z = 09})
		Game.createItem(1498, 1, {x = 33149, y = 32868, z = 09})
	end
	return true
end

action:aid(2003)
action:register()