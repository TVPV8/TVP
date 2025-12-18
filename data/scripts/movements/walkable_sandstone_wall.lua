local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if item:getId() == 464 then
		item:transform(465, 1)
		item:decay()
	elseif item:getId() == 466 then
		item:transform(467, 1)
		item:decay()
	end
end

moveevent:aid(actionIds.sandstoneWall)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onStepOut(creature, item, position, fromPosition)
	if item:getId() == 465 then
		item:transform(464, 1)
		item:decay()
	elseif item:getId() == 467 then
		item:transform(466, 1)
		item:decay()
	end
end

moveevent:aid(actionIds.sandstoneWall)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	if tileitem:getId() == 464 then
		tileitem:transform(465, 1)
		tileitem:decay()
	elseif tileitem:getId() == 466 then
		tileitem:transform(467, 1)
		tileitem:decay()
	end
end

moveevent:aid(actionIds.sandstoneWall)
moveevent:tileItem(true)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onRemoveItem(item, tileitem, position)
	if tileitem:getId() == 465 then
		tileitem:transform(464, 1)
		tileitem:decay()
	elseif tileitem:getId() == 467 then
		tileitem:transform(466, 1)
		tileitem:decay()
	end
end

moveevent:aid(actionIds.sandstoneWall)
moveevent:tileItem(true)
moveevent:register()