local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
		Game.createItem(1739, 1, {x = 32479, y = 31901, z = 05})
		Game.createItem(1491, 1, {x = 32479, y = 31892, z = 03})
	elseif item:getId() == 1946 and not Game.isItemInPosition({x = 32479, y = 31892, z = 03}, 1491) then
		item:transform(1945, 1)
		item:decay()
	end
	return true
end

action:aid(2047)
action:register()