local items = {
	1497, 1498, 1499
}

local event = MoveEvent()

function event.onStepIn(creature, item, position, fromPosition)
	item:remove(1)
	return true
end

for _, id in pairs(items) do
	event:id(id)
end

event:tileItem(true)
event:register()
