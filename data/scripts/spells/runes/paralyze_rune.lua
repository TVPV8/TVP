local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(900)
spell:magicLevel(35)
spell:soul(0)
spell:isPremium(true)
spell:isAggressive(false)
spell:isPremium(true)
spell:name("Paralyze")
spell:vocation("Druid", "Elder Druid")
spell:words("ad,ana, ani")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(900, 2260, 2278, 1)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_RED)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)

local condition = Condition(CONDITION_PARALYZE)
condition:setParameter(CONDITION_PARAM_TICKS, 10000)
condition:setParameter(CONDITION_PARAM_SPEED, -101)
combat:addCondition(condition)

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	if creature:getVocation():getId() ~= 2 and creature:getVocation():getId() ~= 6 then
		creature:sendTextMessage(MESSAGE_STATUS_DEFAULT, Game.getReturnMessage(RETURNVALUE_YOURVOCATIONCANNOTUSETHISSPELL))
		return false
	end

	if not combat:execute(creature, variant) then
		return false
	end

	creature:getPosition():sendMagicEffect(CONST_ME_MAGIC_GREEN)
	return true
end

rune:runeMagicLevel(18)
rune:runeId(2278)
rune:charges(1)
rune:allowFarUse(true)
rune:blockWalls(true)
rune:checkFloor(true)
rune:needTarget(true)
rune:cooldownSpellTime(false)
rune:mana(spell:mana())
rune:vocation("Druid", "Elder Druid")
rune:register()
