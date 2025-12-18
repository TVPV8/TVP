local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32478, y = 31903, z = 03},2628) and Game.isItemInPosition ({x = 32479, y = 31903, z = 03},2634) then 
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 32478, y = 31903, z = 03}, 2628)
		Game.removeItemInPosition({x = 32479, y = 31903, z = 03}, 2634)
		Game.createItem(1386, 1, {x = 32479, y = 31904, z = 03})
	elseif item:getId() == 1945 then 
		Game.sendMagicEffect({x = 32478, y = 31904, z = 03}, 3)
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		Game.removeItemInPosition({x = 32479, y = 31904, z = 03}, 1386)
	end
	return true
end

action:aid(2050)
action:register()