local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32400, y = 32239, z = 06}, 427) then 
		item:transform(1946, 1)
		item:decay()
		Game.transformItemInPosition({x = 32400, y = 32239, z = 06}, 427, 405)
	elseif item:getId() == 1945 then
		item:transform(1946, 1)
		item:decay()
	elseif item:getId() == 1946 and Game.isItemInPosition({x = 32400, y = 32239, z = 06}, 405) then 
		item:transform(1945, 1)
		item:decay()
		Game.transformItemInPosition({x = 32400, y = 32239, z = 06}, 405, 427)
		doRelocate({x = 32400, y = 32239, z = 06},{x = 32400, y = 32239, z = 07})
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
	end
	return true
end

action:aid(2063)
action:register()