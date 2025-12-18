local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	player:teleportTo({x = 32508, y = 32176, z = 14})
	return true
end

action:aid(2080)
action:register()