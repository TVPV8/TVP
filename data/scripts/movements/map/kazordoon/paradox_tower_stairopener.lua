local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and Game.isItemInPosition({x = 32479, y = 31920, z = 07},2782) and Game.isItemInPosition ({x = 32478, y = 31920, z = 07},2782) and Game.isItemInPosition ({x = 32478, y = 31902, z = 07}, 1304) then
		Game.transformItemInPosition({x = 32478, y = 31902, z = 07}, 1304, 1385)
		Game.sendMagicEffect({x = 32478, y = 31902, z = 07}, 3)
	end
end

moveevent:aid(3035)
moveevent:register()