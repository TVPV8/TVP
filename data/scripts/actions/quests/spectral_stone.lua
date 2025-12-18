local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if not target:isItem() then
		return false
	end
	
	if target:getId() == 474 and toPosition.x == 32665 and toPosition.y == 32736 and toPosition.z == 6 and player:getStorageValue(320) == 5 then
		player:setStorageValue(321,1)
		target:getPosition():sendMagicEffect(13)
		return true
	elseif target:getId() == 474 and toPosition.x == 32497 and toPosition.y == 31622 and toPosition.z == 6 and player:getStorageValue(320) == 5 then
		player:setStorageValue(322,1)
		target:getPosition():sendMagicEffect(13)
		return true
	end
	return false
end

action:id(4851) -- spectral stone
action:register()