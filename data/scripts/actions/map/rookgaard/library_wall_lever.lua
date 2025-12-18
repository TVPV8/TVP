local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 32095, y = 32173, z = 08}, 1026)
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 32095, y = 32173, z = 08},{x = 32095, y = 32174, z = 08})
		Game.createItem(1026, 1, {x = 32095, y = 32173, z = 08})
	end

	return true
end

action:aid(2055)
action:register()