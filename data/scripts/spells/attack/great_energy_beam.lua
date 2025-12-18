local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_ENERGYDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_EXPLOSIONHIT)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setArea(createCombatArea(AREA_BEAM8))

function onGetFormulaValues(player, level, magicLevel)
	return player:computeDamage(120, 80)
end

combat:setCallback(CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

spell:needLearn(true)
spell:mana(200)
spell:magicLevel(14)
spell:isAggressive(true)
spell:needDirection(true)
spell:isBlockingWalls(true)
spell:name("Great Energy Beam")
spell:vocation("Sorcerer", "Master Sorcerer")
spell:words("ex,evo, gran, vis, lux")
spell:register()