local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if not target:isItem() then
		return false
	end
	
	if target:getId() == 4138 and player:getStorageValue(305) == 1 then
		item:transform(4870, 1) -- jungle bells plant
		target:getPosition():sendMagicEffect(10)
		return true
	elseif target:getId() == 4149 and player:getStorageValue(305) == 3 then
		item:transform(4872, 1) -- witches cauldrom
		target:getPosition():sendMagicEffect(10)
		return true
	elseif target:getId() == 4142 and player:getStorageValue(305) == 5 then 
		item:transform(4871, 1) -- giant jungle rose
		target:getPosition():sendMagicEffect(10)
		return true
	end
end

action:id(4869) -- empty botanist container
action:register()