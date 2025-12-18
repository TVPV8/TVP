local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32476, y = 31900, z = 04},2682) and Game.isItemInPosition ({x = 32477, y = 31900, z = 04},2676) and Game.isItemInPosition ({x = 32478, y = 31900, z = 04},2679) and Game.isItemInPosition ({x = 32479, y = 31900, z = 04},2674) and Game.isItemInPosition ({x = 32480, y = 31900, z = 04},2681) and Game.isItemInPosition ({x = 32481, y = 31900, z = 04},2678) then
		Game.createItem(1386, 1, {x = 32476, y = 31904, z = 04})
		Game.removeItemInPosition({x = 32476, y = 31900, z = 04}, 2682)
		Game.removeItemInPosition({x = 32477, y = 31900, z = 04}, 2676)
		Game.removeItemInPosition({x = 32478, y = 31900, z = 04}, 2679)
		Game.removeItemInPosition({x = 32479, y = 31900, z = 04}, 2674)
		Game.removeItemInPosition({x = 32480, y = 31900, z = 04}, 2681)
		Game.removeItemInPosition({x = 32481, y = 31900, z = 04}, 2678)
		item:transform(1946, 1)
		item:decay()
	elseif item:getId() == 1945 then
		Game.sendMagicEffect({x = 32479, y = 31905, z = 04}, 3)
	end
	return true
end

action:aid(2049)
action:register()