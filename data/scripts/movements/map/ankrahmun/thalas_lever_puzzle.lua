local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if Game.isItemInPosition({x = 33368, y = 32763, z = 14},4384) and Game.isItemInPosition ({x = 33382, y = 32786, z = 14},4384) and Game.isItemInPosition ({x = 33305, y = 32734, z = 14},4384) and Game.isItemInPosition ({x = 33338, y = 32702, z = 14},4384) and Game.isItemInPosition ({x = 33320, y = 32682, z = 14},4384) and Game.isItemInPosition ({x = 33349, y = 32680, z = 14},4384) and Game.isItemInPosition ({x = 33358, y = 32701, z = 14},4384) and Game.isItemInPosition ({x = 33357, y = 32749, z = 14}, 4384) then 
		doRelocate(item:getPosition(),{x = 33367, y = 32805, z = 14})
		item:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33367, y = 32805, z = 14}, 11)
	else
		doRelocate(item:getPosition(),{x = 33399, y = 32801, z = 14})
		Game.sendMagicEffect({x = 33399, y = 32801, z = 14}, 11)
	end
end

moveevent:aid(3063)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 33368, y = 32763, z = 14},4384) and Game.isItemInPosition ({x = 33382, y = 32786, z = 14},4384) and Game.isItemInPosition ({x = 33305, y = 32734, z = 14},4384) and Game.isItemInPosition ({x = 33338, y = 32702, z = 14},4384) and Game.isItemInPosition ({x = 33320, y = 32682, z = 14},4384) and Game.isItemInPosition ({x = 33349, y = 32680, z = 14},4384) and Game.isItemInPosition ({x = 33358, y = 32701, z = 14},4384) and Game.isItemInPosition ({x = 33357, y = 32749, z = 14}, 4384) then 
		doRelocate(tileitem:getPosition(),{x = 33367, y = 32805, z = 14})
		tileitem:getPosition():sendMagicEffect(11)
		Game.sendMagicEffect({x = 33367, y = 32805, z = 14}, 11)
	else
		doRelocate(tileitem:getPosition(),{x = 33399, y = 32801, z = 14})
		Game.sendMagicEffect({x = 33399, y = 32801, z = 14}, 11)
	end
end

moveevent:aid(3063)
moveevent:tileItem(true)
moveevent:register()