local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and Game.isItemInPosition({x = 32563, y = 31957, z = 01},2229) and Game.isItemInPosition ({x = 32565, y = 31957, z = 01},2229) and Game.isItemInPosition ({x = 32567, y = 31957, z = 01},2229) and Game.isItemInPosition ({x = 32569, y = 31957, z = 01},2229) then 
		doRelocate(item:getPosition(),{x = 32479, y = 31923, z = 07})
		Game.sendMagicEffect({x = 32563, y = 31957, z = 01}, 10)
		Game.sendMagicEffect({x = 32565, y = 31957, z = 01}, 10)
		Game.sendMagicEffect({x = 32567, y = 31957, z = 01}, 10)
		Game.sendMagicEffect({x = 32569, y = 31957, z = 01}, 10)
		Game.removeItemInPosition({x = 32563, y = 31957, z = 01}, 2229)
		Game.removeItemInPosition({x = 32565, y = 31957, z = 01}, 2229)
		Game.removeItemInPosition({x = 32567, y = 31957, z = 01}, 2229)
		Game.removeItemInPosition({x = 32569, y = 31957, z = 01}, 2229)
		Game.createItem(1490, 1, {x = 32563, y = 31957, z = 01})
		Game.createItem(1490, 1, {x = 32565, y = 31957, z = 01})
		Game.createItem(1490, 1, {x = 32567, y = 31957, z = 01})
		Game.createItem(1490, 1, {x = 32569, y = 31957, z = 01})
		Game.sendMagicEffect({x = 32566, y = 31957, z = 01}, 3)
	end
end

moveevent:aid(3049)
moveevent:register()