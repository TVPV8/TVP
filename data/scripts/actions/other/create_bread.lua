local liquidContainers = {
	2005, 1775
}

local millstones = {
	1381, 1382, 1383, 1384
}

local ovens = {
	1786, 1788, 1790, 1792,
}

local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	local itemId = item:getId()
	if itemId == 2692 then
		if target.type == FLUID_WATER and table.contains(liquidContainers, target.itemid) then
			item:remove(1)
			player:addItem(2693, 1)
			target:transform(target.itemid, FLUID_NONE)
			return true
		end
	elseif table.contains(millstones, target.itemid) and item.itemid ~= 2693 then
		item:remove(1)
		player:addItem(2692, 1)
		return true
	elseif table.contains(ovens, target.itemid) then
		if itemId == 2693 then
			item:remove(1)
			Game.createItem(2689, 1, toPosition)
			return true
		end
	end
	return false
end

action:id(2692)
action:id(2694)
action:id(2693)
action:register()
