local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and creature:getPlayer():getStorageValue(8) ~= 0 then 
		Game.sendMagicEffect(item:getPosition(), 15)
	else
		Game.sendMagicEffect(item:getPosition(), 14)
	end
end

moveevent:aid(3028)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	Game.sendMagicEffect(item:getPosition(), 14)
end

moveevent:aid(3028)
moveevent:tileItem(true)
moveevent:register()