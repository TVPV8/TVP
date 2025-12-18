local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and creature:getPlayer():getStorageValue(7) == -1 and Game.isItemInPosition({x = 32214, y = 31850, z = 15}, 1484) then 
		doRelocate(item:getPosition(),{x = 32271, y = 31857, z = 15})
		creature:getPlayer():setStorageValue(7,1)
		Game.sendMagicEffect({x = 32271, y = 31857, z = 15}, 14)
		Game.sendMagicEffect({x = 32217, y = 31846, z = 14}, 12)
		Game.sendMagicEffect({x = 32215, y = 31844, z = 14}, 12)
		Game.sendMagicEffect({x = 32215, y = 31846, z = 14}, 12)
		Game.sendMagicEffect({x = 32217, y = 31847, z = 14}, 12)
		Game.sendMagicEffect({x = 32213, y = 31847, z = 14}, 12)
		Game.sendMagicEffect({x = 32217, y = 31848, z = 14}, 12)
		Game.sendMagicEffect({x = 32215, y = 31848, z = 14}, 12)
		Game.createItem(1491, 1, {x = 32215, y = 31848, z = 15})
		Game.transformItemInPosition({x = 32214, y = 31850, z = 15}, 1484, 1485)
		Game.transformItemInPosition({x = 32215, y = 31850, z = 15}, 1484, 1485)
		Game.transformItemInPosition({x = 32216, y = 31850, z = 15}, 1484, 1485)
		Game.transformItemInPosition({x = 32220, y = 31842, z = 15}, 1945, 1946)
		Game.transformItemInPosition({x = 32220, y = 31843, z = 15}, 1945, 1946)
		Game.transformItemInPosition({x = 32220, y = 31844, z = 15}, 1945, 1946)
		Game.transformItemInPosition({x = 32220, y = 31845, z = 15}, 1945, 1946)
		Game.transformItemInPosition({x = 32220, y = 31846, z = 15}, 1945, 1946)
	else
		doRelocate(item:getPosition(),{x = 32215, y = 31848, z = 15})
		Game.sendMagicEffect({x = 32215, y = 31848, z = 15}, 1)
		doTargetCombatHealth(0, creature, COMBAT_FIREDAMAGE, -55, -55)
	end
end

moveevent:aid(3100)
moveevent:register()