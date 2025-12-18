local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	item:getPosition():sendMagicEffect(23)
	player:setStorageValue(259, 1)
	return true
end

action:aid(2070)
action:register()