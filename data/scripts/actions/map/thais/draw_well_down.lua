local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	player:teleportTo({x = 32354, y = 32131, z = 9})
	return true
end

action:aid(2078)
action:register()