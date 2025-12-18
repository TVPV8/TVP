local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	return onUseMachete(player, item, fromPosition, target, toPosition)
end

action:id(2420, 2442)
action:register()