local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32891, y = 32590, z = 11},4384) and Game.isItemInPosition ({x = 32843, y = 32649, z = 11},4384) and Game.isItemInPosition ({x = 32808, y = 32613, z = 11},4384) and Game.isItemInPosition ({x = 32775, y = 32583, z = 11},4384) and Game.isItemInPosition ({x = 32756, y = 32494, z = 11},4384) and Game.isItemInPosition ({x = 32799, y = 32556, z = 11},4384) and Game.isItemInPosition ({x = 32864, y = 32556, z = 11},3474) then 
		item:transform(1946, 1)
		item:decay()
		Game.sendMagicEffect({x = 32864, y = 32556, z = 11}, 14)
		Game.removeItemInPosition({x = 32864, y = 32556, z = 11}, 3474)
	elseif item:getId() == 1945 and not Game.isItemInPosition({x = 32864, y = 32556, z = 11}, 3474) then
		player:sendCancelMessage("The lever won't budge.")
	elseif item:getId() == 1946 and not Game.isItemInPosition({x = 32864, y = 32556, z = 11}, 3474) then 
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 32864, y = 32556, z = 11},{x = 32864, y = 32557, z = 11})
		Game.sendMagicEffect({x = 32864, y = 32556, z = 11}, 14)
		Game.createItem(3474, 1, {x = 32864, y = 32556, z = 11})
	elseif item:getId() == 1946 and Game.isItemInPosition({x = 32864, y = 32556, z = 11}, 3474) then
		item:transform(1945, 1)
		item:decay()
	end
	return true
end

action:aid(2005)
action:register()