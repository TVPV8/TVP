local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(70)
spell:magicLevel(3)
spell:soul(0)
spell:isAggressive(false)
spell:name("Heavy Magic Missile")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid", "Paladin", "Royal Paladin")
spell:words("ad,ori, gran")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(70, 2260, 2311, 5)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_ENERGYDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_EXPLOSIONHIT)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_FIRE)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setArea(createCombatArea(AREA_SINGLE))

function onGetFormulaValues(player, level, magicLevel)
	return player:computeDamage(30, 10, true)
end

combat:setCallback(CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

rune:runeMagicLevel(1)
rune:runeId(2311)
rune:charges(5)
rune:allowFarUse(true)
rune:blockWalls(true)
rune:checkFloor(true)
rune:needTarget(true)
rune:register()
