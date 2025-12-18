local items = {
	1728, 1729, 1730, 1731, 
	2036, 3900,
}

local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	player:sendTextMessage(MESSAGE_INFO_DESCR, "The time is " .. getFormattedWorldTime() .. ".")
	return true
end

for _, id in ipairs(items) do
	action:id(id)
end

action:register()
