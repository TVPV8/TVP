local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and creature:getPlayer():getStorageValue(64) < 1 then 
		creature:getPlayer():setStorageValue(64,1)
	end
end

moveevent:aid(3032)
moveevent:register()