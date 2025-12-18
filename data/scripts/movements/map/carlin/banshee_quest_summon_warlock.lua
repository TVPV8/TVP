local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and creature:getPlayer():getStorageValue(3) == -1 then 
		Game.createMonster("Warlock", {x = 32216, y = 31841, z = 15}, true)
		Game.createMonster("Warlock", {x = 32216, y = 31834, z = 15}, true)
		creature:getPlayer():setStorageValue(3,1)
	end
end

moveevent:aid(3026)
moveevent:register()