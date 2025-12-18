local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if not target:isItem() then
		return false
	end
	
	if target:getId() == 2147 then
		item:getPosition():sendMagicEffect(14)
		item:transform(2343, 1)
		item:decay()
		target:remove(1)
		return true
	end
	return false
end

action:id(2342) -- helmet of the ancients (no gem)
action:register()
