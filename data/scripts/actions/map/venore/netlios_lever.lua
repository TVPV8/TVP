local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1946 and Game.isItemInPosition({x = 32685, y = 32084, z = 09}, 1284) then 
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 32687, y = 32084, z = 09},{x = 32683, y = 32084, z = 09})
		doRelocate({x = 32686, y = 32084, z = 09},{x = 32683, y = 32084, z = 09})
		doRelocate({x = 32685, y = 32084, z = 09},{x = 32683, y = 32084, z = 09})
		doRelocate({x = 32684, y = 32084, z = 09},{x = 32683, y = 32084, z = 09})
		Game.transformItemInPosition({x = 32687, y = 32084, z = 09}, 1284, 598)
		Game.createItem(4809, 1, {x = 32687, y = 32084, z = 09})
		Game.transformItemInPosition({x = 32686, y = 32084, z = 09}, 1284, 598)
		Game.transformItemInPosition({x = 32685, y = 32084, z = 09}, 1284, 598)
		Game.transformItemInPosition({x = 32684, y = 32084, z = 09}, 1284, 598)
		Game.createItem(4811, 1, {x = 32684, y = 32084, z = 09})
	elseif item:getId() == 1946 then
		item:transform(1945, 1)
		item:decay()
	elseif item:getId() == 1945 and Game.isItemInPosition({x = 32685, y = 32084, z = 09},598) then
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 32684, y = 32084, z = 09}, 4811)
		Game.transformItemInPosition({x = 32684, y = 32084, z = 09}, 598, 1284)
		Game.transformItemInPosition({x = 32685, y = 32084, z = 09}, 598, 1284)
		Game.removeItemInPosition({x = 32687, y = 32084, z = 09}, 4809)
		Game.transformItemInPosition({x = 32687, y = 32084, z = 09}, 598, 1284)
		Game.transformItemInPosition({x = 32686, y = 32084, z = 09}, 598, 1284)
	elseif item:getId() == 1945 then
		item:transform(1946, 1)
		item:decay()
	end
	return true
end

action:aid(2009)
action:register()