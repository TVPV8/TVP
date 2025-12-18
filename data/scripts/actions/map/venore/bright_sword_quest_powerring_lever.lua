local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32594, y = 32214, z = 09},2166) then 
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 32603, y = 32216, z = 09}, 1207)
		Game.removeItemInPosition({x = 32604, y = 32216, z = 09}, 1208)
		doRelocate({x = 32603, y = 32216, z = 09},{x = 32603, y = 32217, z = 09})
		doRelocate({x = 32604, y = 32216, z = 09},{x = 32604, y = 32217, z = 09})
		doRelocate({x = 32593, y = 32216, z = 09},{x = 32592, y = 32216, z = 09})
		doRelocate({x = 32594, y = 32216, z = 09},{x = 32592, y = 32216, z = 09})
		Game.removeItemInPosition({x = 32606, y = 32216, z = 09}, 1026)
		Game.removeItemInPosition({x = 32607, y = 32216, z = 09}, 1026)
		Game.transformItemInPosition({x = 32601, y = 32216, z = 09}, 1026, 1207)
		Game.transformItemInPosition({x = 32602, y = 32216, z = 09}, 1026, 1208)
		Game.createItem(1026, 1, {x = 32594, y = 32216, z = 09})
		Game.createItem(1026, 1, {x = 32593, y = 32216, z = 09})
		Game.createItem(1026, 1, {x = 32603, y = 32216, z = 09})
		Game.createItem(1026, 1, {x = 32604, y = 32216, z = 09})
		Game.removeItemInPosition({x = 32594, y = 32214, z = 09}, 2166)
		Game.sendMagicEffect({x = 32594, y = 32214, z = 09}, 9)
	elseif item:getId() == 1945 then 
		Game.sendMagicEffect({x = 32594, y = 32214, z = 09}, 3)
	elseif item:getId() == 1946 and Game.isItemInPosition({x = 32594, y = 32214, z = 09},2166) then
		item:transform(1945, 1)
		item:decay()
		Game.removeItemInPosition({x = 32593, y = 32216, z = 09}, 1026)
		Game.removeItemInPosition({x = 32594, y = 32216, z = 09}, 1026)
		Game.transformItemInPosition({x = 32601, y = 32216, z = 09}, 1207, 1026)
		Game.transformItemInPosition({x = 32602, y = 32216, z = 09}, 1208, 1026)
		Game.transformItemInPosition({x = 32603, y = 32216, z = 09}, 1026, 1207)
		Game.transformItemInPosition({x = 32604, y = 32216, z = 09}, 1026, 1208)
		Game.createItem(1026, 1, {x = 32606, y = 32216, z = 09})
		Game.createItem(1026, 1, {x = 32607, y = 32216, z = 09})
		Game.removeItemInPosition({x = 32594, y = 32214, z = 09}, 2166)
		Game.sendMagicEffect({x = 32594, y = 32214, z = 09}, 9)
	elseif item:getId() == 1946 then
		Game.sendMagicEffect({x = 32594, y = 32214, z = 09}, 3)
	end
	return true
end

action:aid(2046)
action:register()