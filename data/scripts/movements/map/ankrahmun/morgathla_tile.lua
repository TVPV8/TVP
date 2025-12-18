local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and creature:getPlayer():getStorageValue(293) == 7 then 
		creature:getPlayer():setStorageValue(296, 1)
		item:getPosition():sendMagicEffect(13)
		creature:say("!-! -O- I_I (/( --I Morgathla", TALKTYPE_MONSTER_SAY)
	end
end

moveevent:aid(3014)
moveevent:register()