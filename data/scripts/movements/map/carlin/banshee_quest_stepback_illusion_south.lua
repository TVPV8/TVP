local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() then
		doRelocate(position, {x = position.x, y = 31911, z = 12})
	end
end

moveevent:aid(3006)
moveevent:register()