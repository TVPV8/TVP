local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
		doRelocate({x = 32225, y = 32276, z = 08},{x = 32225, y = 32276, z = 09})
		Game.transformItemInPosition({x = 32225, y = 32276, z = 08}, 351, 369)
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		Game.transformItemInPosition({x = 32225, y = 32276, z = 08}, 369, 351)
	end
	return true
end

action:aid(2064)
action:register()