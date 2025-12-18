local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 33314, y = 31592, z = 15}, 1355)
		doRelocate({x = 33316, y = 31591, z = 15},{x = 33317, y = 31591, z = 15})
		Game.createItem(1387, 1, {x = 33316, y = 31591, z = 15})
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 33314, y = 31592, z = 15},{x = 33315, y = 31592, z = 15})
		Game.createItem(1355, 1, {x = 33314, y = 31592, z = 15})
		Game.removeItemInPosition({x = 33316, y = 31591, z = 15}, 1387)
	end
	return true
end

action:aid(2012)
action:register()