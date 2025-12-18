local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and creature:getPlayer():getStorageValue(8) <= 0 then 
		creature:getPlayer():setStorageValue(8, 1)
	end
	Game.sendMagicEffect(item:getPosition(), 15)
end

moveevent:aid(3027)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	Game.sendMagicEffect(item:getPosition(), 15)
end

moveevent:aid(3027)
moveevent:tileItem(true)
moveevent:register()