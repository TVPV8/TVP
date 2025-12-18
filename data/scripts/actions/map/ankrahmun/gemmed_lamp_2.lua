local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if player:getItemCount(2344) >= 1 and player:getStorageValue(283) == 1 then
		Game.sendMagicEffect({x = 33048, y = 32630, z = 01}, 14)
		player:removeItem(2344, 1)
		Game.createItem(2356, 1, {x = 33048, y = 32631, z = 01})
		player:setStorageValue(283, 2)
	end
	return true
end

action:aid(2069)
action:register()