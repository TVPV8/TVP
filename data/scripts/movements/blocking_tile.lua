local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	creature:teleportTo(fromPosition, true)
end

moveevent:aid(actionIds.blockingTile)
moveevent:register()
