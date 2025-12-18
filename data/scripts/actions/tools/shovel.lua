local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	return onUseShovel(player, item, fromPosition, target, toPosition)
end

action:id(2554)
action:register()