local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 2578 then
		if Tile(item:getPosition()):hasFlag(TILESTATE_PROTECTIONZONE) then
			item:getPosition():sendMagicEffect(CONST_ME_POFF)
			return true
		end

		item:transform(2579)
	else
		item:transform(2578)
		item:getPosition():sendMagicEffect(CONST_ME_POFF)
	end
	return true
end

action:id(2579)
action:id(2578)
action:register()
