local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 32566, y = 32119, z = 07}, 1025)
		Game.transformItemInPosition({x = 32566, y = 32118, z = 07}, 1025, 1029)
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 32566, y = 32119, z = 07},{x = 32567, y = 32119, z = 07})
		Game.createItem(1025, 1, {x = 32566, y = 32119, z = 07})
		Game.transformItemInPosition({x = 32566, y = 32118, z = 07}, 1029, 1025)
	end
	return true
end

action:aid(2036)
action:register()