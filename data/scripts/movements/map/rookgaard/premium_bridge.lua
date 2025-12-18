local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and not creature:getPlayer():isPremium() then
		doRelocate(item:getPosition(),{x = item:getPosition().x + 3, y = item:getPosition().y, z = 07})
		Game.sendMagicEffect(item:getPosition(), 13)
	end
end

moveevent:aid(3052)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(tileitem:getPosition(),{x = tileitem:getPosition().x + 3, y = tileitem:getPosition().y, z = 07})
	Game.sendMagicEffect(item:getPosition(), 13)
end

moveevent:aid(3052)
moveevent:tileItem(true)
moveevent:register()