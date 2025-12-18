local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then
		Game.removeItemInPosition({x = 32145, y = 32101, z = 11}, 1304)
		item:transform(1946, 1)
		item:decay()
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 32145, y = 32101, z = 11},{x = 32145, y = 32102, z = 11})
		Game.createItem(1304, 1, {x = 32145, y = 32101, z = 11})
	end
	return true
end

action:aid(2052)
action:register()