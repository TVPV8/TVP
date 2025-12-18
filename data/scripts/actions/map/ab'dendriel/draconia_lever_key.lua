local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32792, y = 31581, z = 07},1037) then 
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 32792, y = 31581, z = 07}, 1037)
	elseif item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
	elseif item:getId() == 1946 and Game.isItemInPosition({x = 32792, y = 31581, z = 07}, 1037) then
		item:transform(1945, 1)
		item:decay()
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 32792, y = 31581, z = 07},{x = 32792, y = 31582, z = 07})
		Game.createItem(1037, 1, {x = 32792, y = 31581, z = 07})
	end
	return true
end

action:aid(2008)
action:register()