local foods = {
	[2362] = 8, -- carrot
	[2666] = 15, -- meat
	[2667] = 12, -- fish
	[2668] = 10, -- salmon
	[2669] = 17, -- northern pike
	[2670] = 4, -- shrimp
	[2671] = 30, -- ham
	[2672] = 60, -- dragon ham
	[2673] = 5, -- pear
	[2674] = 6, -- red apple
	[2675] = 13, -- orange
	[2676] = 8, -- banana
	[2677] = 1, -- blueberry
	[2678] = 18, -- coconut
	[2679] = 1, -- cherry
	[2680] = 2, -- strawberry
	[2681] = 9, -- grapes
	[2682] = 20, -- melon
	[2683] = 17, -- pumpkin
	[2684] = 8, -- carrot
	[2685] = 6, -- tomato
	[2686] = 9, -- corncob
	[2687] = 2, -- cookie
	[2688] = 2, -- candy cane
	[2689] = 10, -- bread
	[2690] = 3, -- roll
	[2691] = 8, -- brown bread
	[2695] = 6, -- egg
	[2696] = 9, -- cheese
	[2787] = 9, -- white mushroom
	[2788] = 4, -- red mushroom
	[2789] = 22, -- brown mushroom
	[2790] = 30, -- orange mushroom
	[2791] = 9, -- wood mushroom
	[2792] = 6, -- dark mushroom
	[2793] = 12, -- some mushrooms
	[2794] = 3, -- some mushrooms
	[2795] = 36, -- fire mushroom
	[2796] = 5, -- green mushroom
}

local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	local food = foods[item.itemid]
	if not food then
		return false
	end

	local condition = player:getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT)
	if condition and math.floor(condition:getTicks() / 1000) > 1200 - 12 * food then
		player:sendTextMessage(MESSAGE_STATUS_SMALL, "You are full.")
	else
		player:feed(food * 12)
		item:remove(1)
	end
	return true
end

for id in pairs(foods) do
	action:id(id)
end

action:register()
