local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() then
		item:transform(425, 1)
		item:decay()
		Game.sendMagicEffect({x = 32468, y = 32119, z = 14}, 15)
		Game.sendMagicEffect({x = 32482, y = 32170, z = 14}, 15)
		Game.createItem(430, 1, {x = 32482, y = 32170, z = 14})
	end
end

moveevent:aid(3034)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onStepOut(creature, item, position, fromPosition)
	if creature:isPlayer() then 
		item:transform(426, 1)
		item:decay()
		Game.sendMagicEffect({x = 32468, y = 32119, z = 14}, 14)
		Game.sendMagicEffect({x = 32482, y = 32170, z = 14}, 14)
		Game.removeItemInPosition({x = 32482, y = 32170, z = 14}, 430)
	end
end

moveevent:aid(3034)
moveevent:register()