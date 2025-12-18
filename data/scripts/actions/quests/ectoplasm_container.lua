local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if not target:isItem() then
		return false
	end
	
	if target:getId() == 2913 then 
		item:transform(4864, 1) -- full ectoplasm container
		item:decay()
		target:getPosition():sendMagicEffect(12)
		item:getPosition():sendMagicEffect(13)
		return true
	end
end

action:id(4863) -- empty ectoplasm container
action:register()