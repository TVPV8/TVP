local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(120)
spell:magicLevel(9)
spell:soul(0)
spell:isAggressive(false)
spell:name("Great Fireball")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid")
spell:words("ad,ori, gran, flam")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(120, 2260, 2304, 2)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_FIREDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_FIREAREA)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_FIRE)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setArea(createCombatArea(AREA_CIRCLE3X3))

function onGetFormulaValues(player, level, magicLevel)
	return player:computeDamage(50, 15, true)
end

combat:setCallback(CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	return combat:execute(creature, variant, false)
end

rune:runeMagicLevel(4)
rune:runeId(2304)
rune:charges(2)
rune:allowFarUse(true)
rune:blockWalls(true)
rune:checkFloor(true)
rune:isBlocking(true)
rune:isAggressive(true)
rune:register()
