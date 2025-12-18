local traps = {
	[1510] = { -- strange slits
		transformTo = 1511,
		damage = {-60, -60}
	},
	[1513] = { -- spikes
		damage = {-60, -60}
	},
	[2579] = { -- trap
		transformTo = 2578,
		damage = {-30, -30},
		dontDamagePlayers = true,
	},
	[4208] = { -- jungle maw
		transformTo = 4209,
		damage = {-30, -30},
		type = COMBAT_EARTHDAMAGE
	},
}

local stepInTrap = MoveEvent()
local onAddItem = MoveEvent()
local onAddItemCreation = MoveEvent()

function stepInTrap.onStepIn(creature, item, position, fromPosition)
	local trap = traps[item.itemid]
	if not trap then
		return true
	end

	local applyDamage = true
	if trap.dontDamagePlayers then
		if creature:isPlayer() or (creature:getMaster() and creature:getMaster():getPlayer()) then
			applyDamage = false
			position:sendMagicEffect(CONST_ME_POFF)
		end
	end
	
	if applyDamage then
		local tile = Tile(position)
		if not tile:hasFlag(TILESTATE_PROTECTIONZONE) then
			doTargetCombat(0, creature, trap.type or COMBAT_PHYSICALDAMAGE, trap.damage[1], trap.damage[2], CONST_ME_NONE, ORIGIN_NONE, not trap.type and true or false, false, false)
		end
	end

	if trap.transformTo then
		item:transform(trap.transformTo)
		item:decay()
	end
	return true
end

function onAddItem.onAddItem(moveitem, tileitem, pos)
	local trap = traps[tileitem.itemid]
	if not trap then
		return true
	end

	if trap.transformTo then
		tileitem:transform(trap.transformTo)
		tileitem:decay()
	end

	pos:sendMagicEffect(CONST_ME_POFF)
end

function onAddItemCreation.onAddItem(moveitem, tileitem, pos)
	local trap = traps[moveitem.itemid]
	if not trap then
		return true
	end
	
	if trap.transformTo and not moveitem:getParent():isTile() then
		moveitem:transform(trap.transformTo)
		moveitem:decay()
		pos:sendMagicEffect(CONST_ME_POFF)
	end
end

stepInTrap:id(1510, 1513, 2579, 4208)
stepInTrap:register()

onAddItem:id(2579, 4208)
onAddItem:tileItem(true)
onAddItem:register()

onAddItemCreation:id(2579)
onAddItemCreation:register()