local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	return onUseRope(player, item, fromPosition, target, toPosition)
end

action:id(2120)
action:register()