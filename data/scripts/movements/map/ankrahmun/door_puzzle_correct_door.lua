local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	local player = Player(creature)
	player:setStorageValue(260, 1)
end

moveevent:aid(3123)
moveevent:register()