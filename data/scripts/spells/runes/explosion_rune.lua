local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(180)
spell:magicLevel(12)
spell:soul(0)
spell:isAggressive(false)
spell:name("Explosion")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid")
spell:words("ad,evo, mas, hur")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(180, 2260, 2313, 3)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_EXPLOSIONAREA)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_FIRE)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setParameter(COMBAT_PARAM_BLOCKARMOR, true)
combat:setParameter(COMBAT_PARAM_BLOCKSHIELD, false)
combat:setArea(createCombatArea(AREA_CROSS1X1))

function onGetFormulaValues(player, level, magicLevel)
	return player:computeDamage(60, 40)
end

combat:setCallback(CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	return combat:execute(creature, variant, false)
end

rune:runeMagicLevel(6)
rune:runeId(2313)
rune:charges(3)
rune:allowFarUse(true)
rune:isBlocking(true)
rune:register()