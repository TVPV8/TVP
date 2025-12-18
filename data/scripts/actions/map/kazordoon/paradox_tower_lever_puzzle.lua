local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32476, y = 31900, z = 06},1946) and Game.isItemInPosition ({x = 32477, y = 31900, z = 06},1946) and Game.isItemInPosition ({x = 32478, y = 31900, z = 06},1945) and Game.isItemInPosition ({x = 32479, y = 31900, z = 06},1945) and Game.isItemInPosition ({x = 32480, y = 31900, z = 06},1946) and Game.isItemInPosition ({x = 32481, y = 31900, z = 06}, 1945) then 
		item:transform(1946, 1)
		item:decay()
		Game.createItem(1386, 1, {x = 32476, y = 31904, z = 06})
	elseif item:getId() == 1945 then
		Game.sendMagicEffect({x = 32479, y = 31905, z = 06}, 3)
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		Game.removeItemInPosition({x = 32476, y = 31904, z = 06}, 1386)
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		Game.removeItemInPosition({x = 32476, y = 31904, z = 06}, 1386)
	end
	return true
end

action:aid(2048)
action:register()