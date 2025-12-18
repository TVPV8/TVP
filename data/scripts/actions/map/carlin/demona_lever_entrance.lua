local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32483, y = 31633, z = 09}, 383) then
		item:transform(1946, 1)
		item:decay()
	elseif item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
		Game.transformItemInPosition({x = 32483, y = 31633, z = 09}, 355, 383)
	elseif item:getId() == 1946 then
		item:transform(1945, 1)
		item:decay()
	end
	return true
end

action:aid(2016)
action:register()