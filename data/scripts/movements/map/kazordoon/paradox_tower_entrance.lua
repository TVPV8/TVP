local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and Game.isItemInPosition({x = 32478, y = 31902, z = 07}, 1385) then 
		Game.transformItemInPosition({x = 32478, y = 31902, z = 07}, 1385, 1304)
		Game.sendMagicEffect({x = 32478, y = 31902, z = 07}, 3)
	end
end

moveevent:aid(3005)
moveevent:register()