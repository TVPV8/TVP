local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if Game.isItemInPosition({x = 33211, y = 32698, z = 13}, 1061) then
		Game.removeItemInPosition({x = 33211, y = 32698, z = 13}, 1061)
	else
		doRelocate({x = 33211, y = 32698, z = 13}, {x = 33211, y = 32697, z = 13})
		Game.createItem(1061, 1, {x = 33211, y = 32698, z = 13})
	end
	return true
end

action:aid(2002)
action:register()