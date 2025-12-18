local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if Game.isItemInPosition({x = 33145, y = 32862, z = 07}, 2562) and creature:isPlayer() then 
		doRelocate(item:getPosition(),{x = 33151, y = 32864, z = 07})
		item:getPosition():sendMagicEffect(15)
		Game.sendMagicEffect({x = 33151, y = 32864, z = 07}, 15)
	else
		doRelocate(item:getPosition(),{x = 33145, y = 32863, z = 07})
		item:getPosition():sendMagicEffect(15)
		Game.sendMagicEffect({x = 33145, y = 32863, z = 07}, 15)
	end
end

moveevent:aid(3070)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 33145, y = 32863, z = 07})
	item:getPosition():sendMagicEffect(15)
	Game.sendMagicEffect({x = 33145, y = 32863, z = 07}, 15)
end

moveevent:aid(3070)
moveevent:tileItem(true)
moveevent:register()