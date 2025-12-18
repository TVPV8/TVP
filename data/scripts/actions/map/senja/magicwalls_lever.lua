local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		item:remove()
		Game.removeItemInPosition({x = 32186, y = 31626, z = 08}, 1498)
		Game.removeItemInPosition({x = 32187, y = 31626, z = 08}, 1498)
		Game.removeItemInPosition({x = 32188, y = 31626, z = 08}, 1498)
		Game.removeItemInPosition({x = 32189, y = 31626, z = 08}, 1498)
		Game.sendMagicEffect({x = 32180, y = 31633, z = 08}, 3)
		Game.sendMagicEffect({x = 32186, y = 31626, z = 08}, 3)
		Game.sendMagicEffect({x = 32187, y = 31626, z = 08}, 3)
		Game.sendMagicEffect({x = 32188, y = 31626, z = 08}, 3)
		Game.sendMagicEffect({x = 32189, y = 31626, z = 08}, 3)
	end
	return true
end

action:aid(2039)
action:register()