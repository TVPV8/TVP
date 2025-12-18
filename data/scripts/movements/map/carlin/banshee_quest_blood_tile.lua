local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if Game.isItemInPosition({x = 32243, y = 31892, z = 14}, 2016) and item:getFluidType() == FLUID_BLOOD then
		Game.sendMagicEffect({x = 32242, y = 31891, z = 14}, 1)
		Game.sendMagicEffect({x = 32243, y = 31891, z = 14}, 1)
		Game.sendMagicEffect({x = 32242, y = 31892, z = 14}, 1)
		Game.sendMagicEffect({x = 32242, y = 31893, z = 14}, 1)
		Game.sendMagicEffect({x = 32243, y = 31893, z = 14}, 1)
	else
		doRelocate({x = 32243, y = 31892, z = 14},{x = 32244, y = 31892, z = 14})
		Game.sendMagicEffect({x = 32243, y = 31892, z = 14}, 3)
	end
end

moveevent:aid(3030)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if Game.isItemInPosition({x = 32243, y = 31892, z = 14}, 2016) and item:getFluidType() == FLUID_BLOOD then
		Game.sendMagicEffect({x = 32242, y = 31891, z = 14}, 1)
		Game.sendMagicEffect({x = 32243, y = 31891, z = 14}, 1)
		Game.sendMagicEffect({x = 32242, y = 31892, z = 14}, 1)
		Game.sendMagicEffect({x = 32242, y = 31893, z = 14}, 1)
		Game.sendMagicEffect({x = 32243, y = 31893, z = 14}, 1)
	else
		doRelocate({x = 32243, y = 31892, z = 14},{x = 32244, y = 31892, z = 14})
		Game.sendMagicEffect({x = 32243, y = 31892, z = 14}, 3)
	end
end

moveevent:aid(3030)
moveevent:tileItem(true)
moveevent:register()