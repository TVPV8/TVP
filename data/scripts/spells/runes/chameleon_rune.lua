local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(150)
spell:magicLevel(11)
spell:soul(0)
spell:isAggressive(false)
spell:name("Chameleon")
spell:vocation("Druid", "Elder Druid")
spell:words("ad,evo, ina")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(150, 2260, 2291, 1)
end

spell:register()

local condition = Condition(CONDITION_OUTFIT)
condition:setTicks(200000)

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	local position, item = variant:getPosition()
	if position.x == CONTAINER_POSITION then
		local container = creature:getContainerById(position.y - 64)
		if container then
			item = container:getItem(position.z)
		else
			item = creature:getSlotItem(position.y)
		end
	else
		item = Tile(position):getTopDownItem()
	end

	if not item or item.itemid == 0 or not isMoveable(item.uid) then
		creature:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
		creature:getPosition():sendMagicEffect(CONST_ME_POFF)
		return false
	end

	condition:setOutfit({lookTypeEx = item.itemid})
	creature:addCondition(condition)
	creature:getPosition():sendMagicEffect(CONST_ME_MAGIC_RED)
	return true
end

rune:runeMagicLevel(4)
rune:runeId(2291)
rune:charges(1)
rune:allowFarUse(true)
rune:isSelfTarget(true)
rune:isBlocking(true)
rune:isAggressive(false)
rune:register()
