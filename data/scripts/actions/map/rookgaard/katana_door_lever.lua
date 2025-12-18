local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32177, y = 32148, z = 11}, 1211) then 
		item:transform(1946, 1)
		item:decay()
		doRelocate({x = 32177, y = 32148, z = 11},{x = 32178, y = 32148, z = 11})
		Game.transformItemInPosition({x = 32177, y = 32148, z = 11}, 1211, 1209)
	elseif item:getId() == 1945 then
		item:transform(1946, 1)
		item:decay()
		Game.transformItemInPosition({x = 32177, y = 32148, z = 11}, 1210, 1209)
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		Game.transformItemInPosition({x = 32177, y = 32148, z = 11}, 1209, 1211)
	end
	return true
end

action:aid(2051)
action:register()