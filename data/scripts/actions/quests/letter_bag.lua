local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if not target:isItem() then
		return false
	end
	
	if target:getId() == 2334 and toPosition.x == 31948 and toPosition.y == 31711 and toPosition.z == 6 then 
		item:transform(1993, 1) -- red bag
		item:decay()
		player:setStorageValue(244, 2)
		toPosition:sendMagicEffect(19)
		return true
	end
	return false
end

action:id(2330) -- letter bag
action:register()
