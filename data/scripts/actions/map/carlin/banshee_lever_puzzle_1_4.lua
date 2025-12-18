local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1946 and Game.isItemInPosition({x = 32220, y = 31844, z = 15}, 1945) and player:getStorageValue(7) ~= 1 then
		item:transform(1945, 1)
		item:decay()
		item:getPosition():sendMagicEffect(13)
		Game.sendMagicEffect({x = 32217, y = 31845, z = 14}, 12)
		Game.sendMagicEffect({x = 32218, y = 31846, z = 14}, 12)
		Game.sendMagicEffect({x = 32219, y = 31845, z = 14}, 12)
		Game.sendMagicEffect({x = 32220, y = 31846, z = 14}, 12)
		Game.sendMagicEffect({x = 32218, y = 31844, z = 14}, 12)
	elseif item:getId() == 1946 then
		item:getPosition():sendMagicEffect(12)
		doTargetCombatHealth(0, player, COMBAT_FIREDAMAGE, -170, -170)
	end
	return true
end

action:aid(2030)
action:register()