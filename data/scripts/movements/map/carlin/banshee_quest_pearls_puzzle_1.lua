local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if Game.isItemInPosition({x = 32180, y = 31871, z = 15},2144) and Game.isItemInPosition ({x = 32173, y = 31871, z = 15},2143) then 
		doRelocate({x = 32177, y = 31869, z = 15},{x = 32177, y = 31863, z = 15})
		Game.sendMagicEffect({x = 32176, y = 31870, z = 15}, 11)
		Game.removeItemInPosition({x = 32173, y = 31871, z = 15}, 2143)
		Game.removeItemInPosition({x = 32180, y = 31871, z = 15}, 2144)
		Game.sendMagicEffect({x = 32173, y = 31871, z = 15}, 3)
		Game.sendMagicEffect({x = 32180, y = 31871, z = 15}, 3)
	else
		doRelocate({x = 32177, y = 31869, z = 15},{x = 32177, y = 31870, z = 15})
		doTargetCombatHealth(0, creature, COMBAT_POISONDAMAGE, -100, -100)
	end
end

moveevent:aid(3095)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate({x = 32177, y = 31869, z = 15},{x = 32177, y = 31870, z = 15})
end

moveevent:aid(3095)
moveevent:tileItem(true)
moveevent:register()