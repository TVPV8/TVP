local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and Game.isItemInPosition({x = 32104, y = 32082, z = 07},4612) and Game.isItemInPosition ({x = 32102, y = 32084, z = 07},1492) then 
		Game.removeItemInPosition({x = 32101, y = 32085, z = 07}, 2383)
		Game.sendMagicEffect({x = 32101, y = 32085, z = 07}, 14)
		Game.transformItemInPosition({x = 32100, y = 32084, z = 07}, 1492, 1494)
		Game.transformItemInPosition({x = 32101, y = 32084, z = 07}, 1492, 1494)
		Game.transformItemInPosition({x = 32102, y = 32084, z = 07}, 1492, 1494)
		Game.transformItemInPosition({x = 32100, y = 32085, z = 07}, 1492, 1494)
		Game.transformItemInPosition({x = 32102, y = 32085, z = 07}, 1492, 1494)
		Game.transformItemInPosition({x = 32100, y = 32086, z = 07}, 1492, 1494)
		Game.transformItemInPosition({x = 32101, y = 32086, z = 07}, 1492, 1494)
		Game.transformItemInPosition({x = 32102, y = 32086, z = 07}, 1492, 1494)
	end
end

moveevent:aid(3012)
moveevent:register()