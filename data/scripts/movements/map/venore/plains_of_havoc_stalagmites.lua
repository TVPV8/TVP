local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() and Game.isItemInPosition({x = 32771, y = 32297, z = 10},387) then 
		Game.removeItemInPosition({x = 32771, y = 32297, z = 10}, 387)
		Game.transformItemInPosition({x = 32770, y = 32282, z = 10}, 353, 355)
	end
end

moveevent:aid(3001)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onStepOut(creature, item, position, fromPosition)
	if not Game.isItemInPosition({x = 32771, y = 32297, z = 10}, 387) and creature:isPlayer() then
		doRelocate({x = 32771, y = 32297, z = 10},{x = 32771, y = 32296, z = 10})
		Game.createItem(387, 1, {x = 32771, y = 32297, z = 10})
		Game.transformItemInPosition({x = 32770, y = 32282, z = 10}, 355, 353)
	end
end

moveevent:aid(3001)
moveevent:register()