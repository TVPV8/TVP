local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		Game.transformItemInPosition({x = 32313, y = 31928, z = 08}, 1945, 1946)
		Game.sendMagicEffect({x = 32314, y = 31928, z = 08}, 12)
	elseif item:getId() == 1946 then 
		Game.transformItemInPosition({x = 32313, y = 31928, z = 08}, 1946, 1945)
		Game.sendMagicEffect({x = 32314, y = 31928, z = 08}, 12)
	end
	return true
end

action:aid(2038)
action:register()