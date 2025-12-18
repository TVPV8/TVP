local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if not creature:isPlayer() then
		return false
	end

	if creature:getLevel() < item:getAttribute(ITEM_ATTRIBUTE_DOORLEVEL) and not creature:getGroup():getAccess() then
		creature:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Only the worthy may pass.")
		creature:teleportTo(fromPosition, true)
		return false
	end
	
	return true
end

for _, id in pairs(openLevelDoors) do
	moveevent:id(id)
end
moveevent:register()