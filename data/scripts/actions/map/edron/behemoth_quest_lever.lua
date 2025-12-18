local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 33295, y = 31677, z = 15},1304) then
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 33295, y = 31677, z = 15}, 1304)
		Game.removeItemInPosition({x = 33296, y = 31677, z = 15}, 1304)
		Game.removeItemInPosition({x = 33297, y = 31677, z = 15}, 1304)
		Game.removeItemInPosition({x = 33298, y = 31677, z = 15}, 1304)
		Game.removeItemInPosition({x = 33299, y = 31677, z = 15}, 1304)
	elseif item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
	elseif item:getId() == 1946 and Game.isItemInPosition({x = 33295, y = 31677, z = 15}, 1304) then
		item:transform(1945, 1)
		item:decay()
	elseif item:getId() == 1946 then
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 33295, y = 31677, z = 15},{x = 33295, y = 31678, z = 15})
		doRelocate({x = 33296, y = 31677, z = 15},{x = 33296, y = 31678, z = 15})
		doRelocate({x = 33297, y = 31677, z = 15},{x = 33297, y = 31678, z = 15})
		doRelocate({x = 33298, y = 31677, z = 15},{x = 33298, y = 31678, z = 15})
		doRelocate({x = 33299, y = 31677, z = 15},{x = 33299, y = 31678, z = 15})
		Game.createItem(1304, 1, {x = 33295, y = 31677, z = 15})
		Game.createItem(1304, 1, {x = 33296, y = 31677, z = 15})
		Game.createItem(1304, 1, {x = 33297, y = 31677, z = 15})
		Game.createItem(1304, 1, {x = 33298, y = 31677, z = 15})
		Game.createItem(1304, 1, {x = 33299, y = 31677, z = 15})
	end
	return true
end

action:aid(2013)
action:register()