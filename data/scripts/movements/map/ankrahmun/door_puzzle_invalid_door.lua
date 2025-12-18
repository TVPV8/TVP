local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	local player = Player(creature)
	player:setStorageValue(260, 0)
end

moveevent:aid(3122)
moveevent:register()