local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	player:teleportTo({x = 32172, y = 32439, z = 8})
	return true
end

action:aid(2079)
action:register()