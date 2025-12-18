local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() then 
		creature:getPlayer():setStorageValue(272,1)
		item:getPosition():sendMagicEffect(13)
	end
end

moveevent:aid(3078)
moveevent:register()