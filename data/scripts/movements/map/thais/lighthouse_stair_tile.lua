local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() then 
		item:transform(425, 1)
		item:decay()
		Game.transformItemInPosition({x = 32225, y = 32282, z = 09}, 424, 433)
	end
end

moveevent:aid(3033)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onStepOut(creature, item, position, fromPosition)
	if creature:isPlayer() then 
		item:transform(426, 1)
		item:decay()
		Game.transformItemInPosition({x = 32225, y = 32282, z = 09}, 433, 424)
	end
end

moveevent:aid(3033)
moveevent:register()