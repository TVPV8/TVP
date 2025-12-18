local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_ENERGYDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_TELEPORT)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setArea(createCombatArea(AREA_SQUAREWAVE5))

function onGetFormulaValues(player, level, magicLevel)
	return player:computeDamage(150, 50)
end

combat:setCallback(CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

spell:needLearn(true)
spell:mana(250)
spell:magicLevel(20)
spell:isPremium(true)
spell:isAggressive(true)
spell:needDirection(true)
spell:isBlockingWalls(true)
spell:name("Energy Wave")
spell:vocation("Sorcerer", "Master Sorcerer")
spell:words("ex,evo, mort, hur")
spell:register()