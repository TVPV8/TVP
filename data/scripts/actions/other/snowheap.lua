local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	Game.createItem(2111, 1, item:getPosition())
	return true
end

action:id(486)
action:register()
