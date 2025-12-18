local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if target:getId() == 4859 and player:getStorageValue(297) == -1 then 
		player:setStorageValue(297, 1)
		target:getPosition():sendMagicEffect(1)
		target:transform(4860, 1)
		target:decay()
		return true
	elseif target:getId() == 4859 and player:getStorageValue(297) == 1 then 
		player:setStorageValue(297, 2)
		target:getPosition():sendMagicEffect(1)
		target:transform(4860, 1)
		target:decay()
		return true
	elseif target:getId() == 4859 and player:getStorageValue(297) == 2 then 
		player:setStorageValue(297, 3)
		target:getPosition():sendMagicEffect(1)
		target:transform(4860, 3)
		target:decay()
		return true
	elseif target:getId() == 1209 and toPosition.x == 32680 and toPosition.y == 32083 and toPosition.z == 09 then 
		Game.transformItemInPosition({x = 32680, y = 32083, z = 09}, 1209, 1211) -- door from locked, to wide opened
		return true
	elseif target:getId() == 2593 and toPosition.x == 32013 and toPosition.y == 31562 and toPosition.z == 04 and player:getStorageValue(228) == 1 then
		Game.sendMagicEffect({x = 32013, y = 31562, z = 04}, 15)
		player:setStorageValue(228, 2)
		return true
	elseif target:getId() == 2593 and toPosition.x == 32013 and toPosition.y == 31562 and toPosition.z == 04 then
		Game.sendMagicEffect({x = 32013, y = 31562, z = 04}, 3)
		return true
	end
	return destroyItem(player, target, toPosition)
end

action:id(2416)
action:register()
