local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_FIREDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_FIREAREA)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setArea(createCombatArea(AREA_WAVE4))

function onGetFormulaValues(player, level, magicLevel)
	return player:computeDamage(30, 10, true)
end

combat:setCallback(CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

spell:needLearn(true)
spell:mana(80)
spell:magicLevel(7)
spell:isAggressive(true)
spell:needDirection(true)
spell:isBlockingWalls(true)
spell:name("Fire Wave")
spell:vocation("Sorcerer", "Master Sorcerer")
spell:words("ex,evo, flam, hur")
spell:register()