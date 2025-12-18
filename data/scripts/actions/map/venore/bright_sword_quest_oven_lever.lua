local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		item:transform(1946, 1)
		item:decay()
		doRelocate({x = 32623, y = 32189, z = 09},{x = 32623, y = 32190, z = 09}, true)
		doRelocate({x = 32623, y = 32188, z = 09},{x = 32623, y = 32189, z = 09}, true)
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
		doRelocate({x = 32623, y = 32188, z = 09},{x = 32622, y = 32189, z = 09}, true)
		doRelocate({x = 32623, y = 32189, z = 09},{x = 32623, y = 32188, z = 09}, true)
	end
	return true
end

action:aid(2045)
action:register()