local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and creature:getPlayer():getStorageValue(44) < 1 then 
		creature:getPlayer():setStorageValue(44,1)
		Game.sendMagicEffect({x = 32480, y = 31900, z = 01}, 7)
	end
end

moveevent:aid(3045)
moveevent:register()