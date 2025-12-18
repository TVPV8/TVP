local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 2058 and Game.isItemInPosition({x = 33151, y = 32866, z = 08},1100) then 
		Game.removeItemInPosition({x = 33151, y = 32866, z = 08}, 1100)
		Game.sendMagicEffect({x = 33151, y = 32862, z = 07}, 14)
	elseif item:getId() == 2058 then 
		Game.sendMagicEffect({x = 33151, y = 32862, z = 07}, 3)
	end
	return true
end

action:aid(2043)
action:register()