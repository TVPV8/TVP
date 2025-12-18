local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32568, y = 32078, z = 12},1547) and Game.isItemInPosition ({x = 32569, y = 32078, z = 12},1547) then
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 32568, y = 32078, z = 12}, 1547)
		Game.removeItemInPosition({x = 32569, y = 32078, z = 12}, 1547)
	elseif item:getId() == 1946 and Game.isItemInPosition({x = 32568, y = 32078, z = 12},1547) and Game.isItemInPosition ({x = 32569, y = 32078, z = 12}, 1547) then 
		item:transform(1945, 1)
		item:decay()
	elseif item:getId() == 1946 then
		item:transform(1945, 1)
		item:decay()
		Game.createItem(1547, 1, {x = 32568, y = 32078, z = 12})
		Game.createItem(1547, 1, {x = 32569, y = 32078, z = 12})
	end
	return true
end

action:aid(2011)
action:register()