local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	return onUseKnife(player, item, fromPosition, target, toPosition)
end

action:id(2566)
action:register()