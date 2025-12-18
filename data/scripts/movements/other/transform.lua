local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	item:transform(item.itemid + 1)
	item:decay()
	doRelocate(position, {x = position.x, y = position.y, z = position.z + 1})
	return true
end

moveevent:id(293, 461, 3310) -- grass hole, trapdoor and pitfall
moveevent:register()
