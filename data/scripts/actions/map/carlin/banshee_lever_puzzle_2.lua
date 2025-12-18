local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32313, y = 31976, z = 13}, 1423) then 
		item:transform(1946, 1)
		item:decay()
		Game.transformItemInPosition({x = 32313, y = 31976, z = 13}, 1423, 1421)
	elseif item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
	elseif item:getId() == 1946 and Game.isItemInPosition({x = 32313, y = 31976, z = 13}, 1421) then
		item:transform(1945, 1)
		item:decay()
		Game.transformItemInPosition({x = 32313, y = 31976, z = 13}, 1421, 1423)
	elseif item:getId() == 1946 then
		item:transform(1945, 1)
		item:decay()
	end
	return true
end

action:aid(2020)
action:register()