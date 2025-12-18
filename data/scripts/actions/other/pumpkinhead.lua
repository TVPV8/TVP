local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if not target:isItem() then
		return false
	end
	
	if target:getId() == 2047 then
		item:transform(2097, 1)
		item:decay()
		target:remove()
		return true
	end
	return false
end

action:id(2096)
action:register()
