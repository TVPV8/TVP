local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and creature:getPlayer():getLevel() < 2 then
		doRelocate(item:getPosition(),{x = item:getPosition().x - 1, y = 32176, z = 07})
		Game.sendMagicEffect({x = item:getPosition().x - 1, y = 32176, z = 07}, 13)
	end
end

moveevent:aid(3051)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(tileitem:getPosition(),{x = tileitem:getPosition().x - 1, y = 32176, z = 07})
	Game.sendMagicEffect({x = tileitem:getPosition().x - 1, y = 32176, z = 07}, 13)
end

moveevent:aid(3051)
moveevent:tileItem(true)
moveevent:register()