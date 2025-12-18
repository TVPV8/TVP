local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32259, y = 31890, z = 10},1498) then
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 32259, y = 31890, z = 10}, 1498)
	elseif item:getId() == 1945 then
		item:transform(1946, 1)
		item:decay()
	elseif item:getId() == 1946 and Game.isItemInPosition({x = 32259, y = 31890, z = 10}, 1498) then 
		item:transform(1945, 1)
		item:decay()
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 32259, y = 31890, z = 10},{x = 32259, y = 31889, z = 10})
		Game.createItem(1498, 1, {x = 32259, y = 31890, z = 10})
	end
	return true
end

action:aid(2026)
action:register()