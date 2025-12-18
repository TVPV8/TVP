local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if Game.isItemInPosition({x = 32816, y = 31601, z = 09},2319) and creature:isPlayer() then 
		doRelocate(item:getPosition(),{x = 32701, y = 31639, z = 06})
		Game.removeItemInPosition({x = 32816, y = 31601, z = 09}, 2319)
		Game.sendMagicEffect({x = 32701, y = 31639, z = 06}, 11)
		Game.sendMagicEffect({x = 32816, y = 31601, z = 09}, 14)
		creature:getPlayer():setStorageValue(65,0)
		creature:getPlayer():setStorageValue(66,0)
	else
		doRelocate(item:getPosition(),{x = 32818, y = 31599, z = 09})
		Game.sendMagicEffect({x = 32818, y = 31599, z = 09}, 11)
	end
end

moveevent:aid(3103)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 32818, y = 31599, z = 09})
	Game.sendMagicEffect({x = 32818, y = 31599, z = 09}, 11)
end

moveevent:aid(3103)
moveevent:tileItem(true)
moveevent:register()