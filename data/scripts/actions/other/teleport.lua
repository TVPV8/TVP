local items = {
	430, 1386, 3678,
}

local upFloorIds = {
	1386, 3678
}

local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if table.contains(upFloorIds, item.itemid) then
		fromPosition:moveUpstairs()
	else
		fromPosition.z = fromPosition.z + 1
	end

	if player:isPzLocked() and Tile(fromPosition):hasFlag(TILESTATE_PROTECTIONZONE) then
		player:sendCancelMessage(RETURNVALUE_PLAYERISPZLOCKED)
		return true
	end

	player:teleportTo(fromPosition, false)
	return true
end

for _, id in ipairs(items) do
	action:id(id)
end

action:register()
