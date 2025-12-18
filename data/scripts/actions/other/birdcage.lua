local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if math.random(100) == 1 then
		item:transform(2094)
	else
		item:getPosition():sendMagicEffect(CONST_ME_SOUND_YELLOW)
	end
	return true
end

action:id(2095)
action:register()
