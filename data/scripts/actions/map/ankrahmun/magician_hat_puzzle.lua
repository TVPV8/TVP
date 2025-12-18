local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and math.random(1, 100) <= 70 then 
		item:transform(1946, 1)
		item:decay()
		Game.removeItemsInPosition({x = item:getPosition().x - 1, y = item:getPosition().y, z = 14})
		Game.createItem(2355, 1, {x = item:getPosition().x - 1, y = item:getPosition().y, z = 14})
		doTargetCombatHealth(0, player, COMBAT_FIREDAMAGE, -200, -200)
	elseif item:getId() == 1945 then
		item:transform(1946, 1)
		item:decay()
		Game.removeItemsInPosition({x = item:getPosition().x - 1, y = item:getPosition().y, z = 14})
		Game.createItem(2684, 1, {x = 33117, y = item:getPosition().y, z = 14})
		player:setStorageValue(258, 1)
		Game.sendMagicEffect({x = 33122, y = 32765, z = 14}, 15)
		Game.sendMagicEffect({x = 33117, y = 32761, z = 14}, 15)
		Game.sendMagicEffect({x = 33117, y = 32762, z = 14}, 15)
		Game.sendMagicEffect({x = 33117, y = 32763, z = 14}, 15)
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		Game.removeItemsInPosition({x = item:getPosition().x - 1, y = item:getPosition().y, z = 14})
		Game.createItem(2662, 1, {x = item:getPosition().x - 1, y = item:getPosition().y, z = 14})
	end
	return true
end

action:aid(2004)
action:register()