local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32266, y = 31860, z = 11},1498) then 
		Game.removeItemInPosition({x = 32266, y = 31860, z = 11}, 1498)
		Game.transformItemInPosition({x = 32266, y = 31860, z = 11}, 407, 408)
		item:transform(1946, 1)
		item:decay()
	elseif item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
	elseif item:getId() == 1946 and Game.isItemInPosition({x = 32266, y = 31860, z = 11}, 1498) then
		item:transform(1945, 1)
		item:decay()
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		Game.transformItemInPosition({x = 32266, y = 31860, z = 11}, 408, 407)
		Game.createItem(1498, 1, {x = 32266, y = 31860, z = 11})
	end
	return true
end

action:aid(2025)
action:register()