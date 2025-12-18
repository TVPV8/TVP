local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if not target:isItem() then
		return false
	end
	
	if target:getActionId() == actionIds.destroyableStone then
		if math.random(1, 100) <= 40 then
			target:remove()
			toPosition:sendMagicEffect(3)
		else
			doTargetCombatHealth(0, player, COMBAT_PHYSICALDAMAGE, -50, -50)
			toPosition:sendMagicEffect(3)
		end
		return true
	end
	
	return onUsePick(player, item, fromPosition, target, toPosition)
end

action:id(2553)
action:register()