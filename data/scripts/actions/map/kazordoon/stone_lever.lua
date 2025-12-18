local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 32604, y = 31905, z = 03}, 1302)
		Game.removeItemInPosition({x = 32605, y = 31905, z = 03}, 1303)
		Game.removeItemInPosition({x = 32604, y = 31904, z = 03}, 1300)
		Game.removeItemInPosition({x = 32605, y = 31904, z = 03}, 1301)
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 32604, y = 31904, z = 03},{x = 32604, y = 31906, z = 03})
		doRelocate({x = 32604, y = 31905, z = 03},{x = 32604, y = 31906, z = 03})
		doRelocate({x = 32605, y = 31904, z = 03},{x = 32605, y = 31906, z = 03})
		doRelocate({x = 32605, y = 31905, z = 03},{x = 32605, y = 31906, z = 03})
		Game.createItem(1300, 1, {x = 32604, y = 31904, z = 03})
		Game.createItem(1302, 1, {x = 32604, y = 31905, z = 03})
		Game.createItem(1301, 1, {x = 32605, y = 31904, z = 03})
		Game.createItem(1303, 1, {x = 32605, y = 31905, z = 03})
	end
	return true
end

action:aid(2033)
action:register()