local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(100)
spell:magicLevel(10)
spell:soul(0)
spell:isAggressive(false)
spell:name("Convince Creature")
spell:vocation("Druid", "Elder Druid")
spell:words("ad,eta, sio")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(100, 2260, 2290, 1)
end

spell:register()

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	local target = Creature(variant:getNumber())
	if not target or not target:isMonster() then
		creature:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
		creature:getPosition():sendMagicEffect(CONST_ME_POFF)
		return false
	end

	local monsterType = target:getType()
	if not creature:hasFlag(PlayerFlag_CanConvinceAll) then
		if not monsterType:isConvinceable() then
			creature:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
			creature:getPosition():sendMagicEffect(CONST_ME_POFF)
			return false
		end

		if #creature:getSummons() >= 2 then
			creature:sendCancelMessage("You cannot control more creatures.")
			creature:getPosition():sendMagicEffect(CONST_ME_POFF)
			return false
		end
	end

	local manaCost = target:getType():getManaCost()
	if creature:getMana() < manaCost and not creature:hasFlag(PlayerFlag_HasInfiniteMana) then
		creature:sendCancelMessage(RETURNVALUE_NOTENOUGHMANA)
		creature:getPosition():sendMagicEffect(CONST_ME_POFF)
		return false
	end

	creature:addMana(-manaCost)
	creature:addManaSpent(manaCost)
	creature:addSummon(target)
	creature:getPosition():sendMagicEffect(CONST_ME_MAGIC_BLUE)
	target:getPosition():sendMagicEffect(CONST_ME_HITAREA)
	return true
end

rune:runeMagicLevel(5)
rune:runeId(2290)
rune:charges(1)
rune:allowFarUse(true)
rune:blockWalls(true)
rune:checkFloor(true)
rune:needTarget(true)
rune:register()
