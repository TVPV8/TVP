local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 33171, y = 31897, z = 08}, 1285)
	elseif item:getId() == 1946 then
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 33171, y = 31897, z = 08},{x = 33171, y = 31898, z = 08})
		Game.createItem(1285, 1, {x = 33171, y = 31897, z = 08})
	end
	return true
end

action:aid(2014)
action:register()