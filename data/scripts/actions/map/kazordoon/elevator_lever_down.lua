local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		doRelocate({x = 32636, y = 31881, z = 07},{x = 32636, y = 31881, z = 02})
		item:transform(1946, 1)
		item:decay()
		Game.sendMagicEffect({x = 32636, y = 31881, z = 07}, 3)
		Game.sendMagicEffect({x = 32636, y = 31881, z = 02}, 3)
	elseif item:getId() == 1945 then
		doRelocate({x = 32636, y = 31881, z = 07},{x = 32636, y = 31881, z = 02})
		item:transform(1946, 1)
		item:decay()
		Game.sendMagicEffect({x = 32636, y = 31881, z = 02}, 3)
		Game.sendMagicEffect({x = 32636, y = 31881, z = 07}, 3)
	elseif item:getId() == 1946 then
		doRelocate({x = 32636, y = 31881, z = 02},{x = 32636, y = 31881, z = 07})
		item:transform(1945, 1)
		item:decay()
		Game.sendMagicEffect({x = 32636, y = 31881, z = 07}, 3)
		Game.sendMagicEffect({x = 32636, y = 31881, z = 02}, 3)
	elseif item:getId() == 1946 then
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 32636, y = 31881, z = 02},{x = 32636, y = 31881, z = 07})
		Game.sendMagicEffect({x = 32636, y = 31881, z = 02}, 3)
		Game.sendMagicEffect({x = 32636, y = 31881, z = 07}, 3)
	end
	return true
end

action:aid(2032)
action:register()