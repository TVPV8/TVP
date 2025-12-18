local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() then 
		doRelocate(position, {x = position.x, y = 31886, z = 12})
	end
end

moveevent:aid(3007)
moveevent:register()