local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 and Game.isItemInPosition({x = 32915, y = 32076, z = 06},386) then 
		Game.removeItemInPosition({x = 32915, y = 32076, z = 06}, 386)
		Game.removeItemInPosition({x = 32915, y = 32080, z = 06}, 386)
		item:transform(1946, 1)
		item:decay()
	elseif item:getId() == 1945 then
		item:transform(1946, 1)
		item:decay()
	elseif item:getId() == 1946 and not Game.isItemInPosition({x = 32915, y = 32076, z = 06}, 386) then 
		doRelocate({x = 32915, y = 32076, z = 06},{x = 32916, y = 32076, z = 06})
		doRelocate({x = 32915, y = 32080, z = 06},{x = 32916, y = 32080, z = 06})
		Game.createItem(386, 1, {x = 32915, y = 32076, z = 06})
		Game.createItem(386, 1, {x = 32915, y = 32080, z = 06})
		item:transform(1945, 1)
		item:decay()
	elseif item:getId() == 1946 then 
		item:transform(1945, 1)
		item:decay()
	end
	return true
end

action:aid(2040)
action:register()