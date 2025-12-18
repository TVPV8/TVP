local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
		doRelocate({x = 32426, y = 32202, z = 14},{x = 32426, y = 32200, z = 14})
		doRelocate({x = 32426, y = 32201, z = 14},{x = 32426, y = 32200, z = 14})
		doRelocate({x = 32427, y = 32202, z = 14},{x = 32427, y = 32200, z = 14})
		doRelocate({x = 32427, y = 32201, z = 14},{x = 32427, y = 32200, z = 14})
		Game.removeItemInPosition({x = 32426, y = 32202, z = 14}, 1284)
		Game.removeItemInPosition({x = 32426, y = 32201, z = 14}, 1284)
		Game.removeItemInPosition({x = 32427, y = 32202, z = 14}, 1284)
		Game.removeItemInPosition({x = 32427, y = 32201, z = 14}, 1284)
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		Game.createTile({x = 32426, y = 32201, z = 14}, true)
		Game.createTile({x = 32427, y = 32201, z = 14}, true)
		Game.createTile({x = 32426, y = 32202, z = 14}, true)
		Game.createTile({x = 32427, y = 32202, z = 14}, true)
		Game.createItem(1284, 1, {x = 32426, y = 32201, z = 14})
		Game.createItem(1284, 1, {x = 32427, y = 32201, z = 14})
		Game.createItem(1284, 1, {x = 32426, y = 32202, z = 14})
		Game.createItem(1284, 1, {x = 32427, y = 32202, z = 14})
	end
	return true
end

action:aid(2037)
action:register()