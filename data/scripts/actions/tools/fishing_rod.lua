local waterItems = {
	[4608] = {canFish = true, transformId = 4620},
	[4609] = {canFish = true, transformId = 4621},
	[4610] = {canFish = true, transformId = 4622},
	[4611] = {canFish = true, transformId = 4623},
	[4612] = {canFish = true, transformId = 4624},
	[4613] = {canFish = true, transformId = 4625},
	[490] = {canFish = true, transformId = 492},
	[4820] = {canFish = true, transformId = 4620},
	[4821] = {canFish = true, transformId = 4621},
	[4822] = {canFish = true, transformId = 4622},
	[4823] = {canFish = true, transformId = 4623},
	[4824] = {canFish = true, transformId = 4624},
	[4825] = {canFish = true, transformId = 4625},
	[491] = {canFish = false},
	[492] = {canFish = false},
	[493] = {canFish = false},
	[4614] = {canFish = false},
	[4615] = {canFish = false},
	[4616] = {canFish = false},
	[4617] = {canFish = false},
	[4618] = {canFish = false},
	[4619] = {canFish = false},
	[4620] = {canFish = false},
	[4621] = {canFish = false},
	[4622] = {canFish = false},
	[4623] = {canFish = false},
	[4624] = {canFish = false},
	[4625] = {canFish = false},
	[4625] = {canFish = false},
	[4664] = {canFish = false},
	[4665] = {canFish = false},
	[4666] = {canFish = false},
	[618] = {canFish = false},
	[619] = {canFish = false},
	[620] = {canFish = false},
	[621] = {canFish = false},
	[622] = {canFish = false},
	[623] = {canFish = false},
	[624] = {canFish = false},
	[625] = {canFish = false},
	[626] = {canFish = false},
	[627] = {canFish = false},
	[628] = {canFish = false},
	[629] = {canFish = false},
}

local useWorms = false

local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	local targetId = target.itemid
	local water = waterItems[targetId]
	
	if not water then
		return false
	end

	toPosition:sendMagicEffect(CONST_ME_LOSEENERGY)

	if water.canFish then
		player:addSkillTries(SKILL_FISHING, 1)
	end
	
	if water.canFish and math.random(1, 100) <= math.min(math.max(10 + (player:getEffectiveSkillLevel(SKILL_FISHING) - 10) * 0.597, 10), 50) then
		if useWorms and not player:removeItem(3976, 1) then
			return true
		end
			
		local parent = item:getParent()
		if not parent:addItem(2667, 1) then
			Tile(item:getPosition()):addItem(2667, 1)
		end

		target:transform(water.transformId)
		target:decay()
	end
	return true
end

action:id(2580)
action:allowFarUse(true)
action:register()