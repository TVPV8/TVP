local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		Game.transformItemInPosition({x = 32605, y = 31902, z = 04}, 431, 427)
		item:transform(1946, 1)
		item:decay()
		doRelocate({x = 32605, y = 31902, z = 04},{x = 32605, y = 31902, z = 05})
	elseif item:getId() == 1946 then 
		Game.transformItemInPosition({x = 32605, y = 31902, z = 04}, 427, 431)
		item:transform(1945, 1)
		item:decay()
	end
	return true
end

action:aid(2034)
action:register()