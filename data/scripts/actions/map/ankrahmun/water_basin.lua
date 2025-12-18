local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if player:getStorageValue(287) == 1 then
		player:setStorageValue(287, 2)
		 Game.createItem(2346, 1, player:getPosition())
		 Game.sendMagicEffect(item:getPosition(), 2)
	end
	return true
end

action:aid(2067)
action:register()