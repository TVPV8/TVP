local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32673, y = 32085, z = 08},425) and Game.isItemInPosition ({x = 32669, y = 32089, z = 08},425) and Game.isItemInPosition ({x = 32673, y = 32093, z = 08},425) and Game.isItemInPosition ({x = 32677, y = 32089, z = 08},425) and Game.isItemInPosition ({x = 32673, y = 32083, z = 08},2455) and Game.isItemInPosition ({x = 32667, y = 32089, z = 08},2674) and Game.isItemInPosition ({x = 32673, y = 32094, z = 08},2376) and Game.isItemInPosition ({x = 32679, y = 32089, z = 08},2175) then
		item:transform(1946, 1)
		item:decay()
		Game.removeItemInPosition({x = 32673, y = 32083, z = 08}, 2455)
		Game.removeItemInPosition({x = 32667, y = 32089, z = 08}, 2674)
		Game.removeItemInPosition({x = 32673, y = 32094, z = 08}, 2376)
		Game.removeItemInPosition({x = 32679, y = 32089, z = 08}, 2175)
		Game.sendMagicEffect({x = 32673, y = 32083, z = 08}, 11)
		Game.sendMagicEffect({x = 32667, y = 32089, z = 08}, 11)
		Game.sendMagicEffect({x = 32673, y = 32094, z = 08}, 11)
		Game.sendMagicEffect({x = 32679, y = 32089, z = 08}, 11)
		doRelocate({x = 32673, y = 32093, z = 08},{x = 32671, y = 32069, z = 08})
		doRelocate({x = 32669, y = 32089, z = 08},{x = 32672, y = 32069, z = 08})
		doRelocate({x = 32673, y = 32085, z = 08},{x = 32671, y = 32070, z = 08})
		doRelocate({x = 32677, y = 32089, z = 08},{x = 32672, y = 32070, z = 08})
		Game.sendMagicEffect({x = 32671, y = 32069, z = 08}, 11)
		Game.sendMagicEffect({x = 32672, y = 32069, z = 08}, 11)
		Game.sendMagicEffect({x = 32671, y = 32070, z = 08}, 11)
		Game.sendMagicEffect({x = 32672, y = 32070, z = 08}, 11)
	elseif item:getId() == 1946 then
		item:transform(1945, 1)
		item:decay()
	end
	return true
end

action:aid(2010)
action:register()