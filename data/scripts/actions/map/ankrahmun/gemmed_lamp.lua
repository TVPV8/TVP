local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if player:getItemCount(2344) >= 1 and player:getStorageValue(288) == 1 then
		Game.sendMagicEffect({x = 33094, y = 32524, z = 01}, 14)
		player:removeItem(2344, 1)
		Game.createItem(2356, 1, {x = 33095, y = 32524, z = 01})
		player:setStorageValue(288, 2)
	end
	return true
end

action:aid(2068)
action:register()