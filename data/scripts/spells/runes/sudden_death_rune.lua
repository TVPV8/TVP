local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(220)
spell:magicLevel(25)
spell:soul(0)
spell:isAggressive(false)
spell:name("Sudden Death")
spell:vocation("Sorcerer", "Master Sorcerer")
spell:words("ad,ori, vita, vis")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(220, 2260, 2268, 1)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MORTAREA)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_DEATH)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setParameter(COMBAT_PARAM_BLOCKARMOR, true)
combat:setParameter(COMBAT_PARAM_BLOCKSHIELD, false)
combat:setArea(createCombatArea(AREA_SINGLE))

function onGetFormulaValues(player, level, magicLevel)
	return player:computeDamage(150, 20)
end

combat:setCallback(CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

rune:runeMagicLevel(15)
rune:runeId(2268)
rune:charges(1)
rune:allowFarUse(true)
rune:blockWalls(true)
rune:checkFloor(true)
rune:isBlocking(true)
rune:needTarget(true)
rune:isAggressive(true)
rune:register()
