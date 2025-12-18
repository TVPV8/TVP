local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if not target:isItem() then
		return false
	end
	
	if target:getId() == 2006 and target:getFluidType() == FLUID_OIL then 
		target:transform(target:getId(), FLUID_NONE)
		item:transform(2044, 1) -- brand new lamp
		item:decay()
		return true
	end
	return false
end

action:id(2046) -- used lamp
action:register()
