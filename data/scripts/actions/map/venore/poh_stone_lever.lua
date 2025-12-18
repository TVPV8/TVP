local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 32780, y = 32231, z = 08}, 387)
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 32780, y = 32231, z = 08},{x = 32780, y = 32232, z = 08})
		Game.createItem(387, 1, {x = 32780, y = 32231, z = 08})
	end
	return true
end

action:aid(2041)
action:register()