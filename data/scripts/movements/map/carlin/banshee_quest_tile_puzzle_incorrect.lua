local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and creature:getPlayer():getStorageValue(8) ~= 0 then 
		creature:getPlayer():setStorageValue(8, 0)
	end
	Game.sendMagicEffect(item:getPosition(), 14)
end

moveevent:aid(3029)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	Game.sendMagicEffect(item:getPosition(), 14)
end

moveevent:aid(3029)
moveevent:tileItem(true)
moveevent:register()