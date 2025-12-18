local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	return onUseScythe(player, item, fromPosition, target, toPosition)
end

action:id(2550)
action:register()