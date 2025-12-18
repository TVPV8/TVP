local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_HEALING)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
combat:setParameter(COMBAT_PARAM_DISPEL, CONDITION_PARALYZE)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, false)
combat:setParameter(COMBAT_PARAM_TARGETCASTERORTOPMOST, true)

function onGetFormulaValues(player, level, magicLevel)
	return player:computeHealing(250, 50, true)
end

combat:setCallback(CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

spell:isSelfTarget(true)
spell:needLearn(true)
spell:mana(80)
spell:magicLevel(8)
spell:isAggressive(false)
spell:name("Ultimate Healing")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid", "Paladin", "Royal Paladin")
spell:words("ex,ura, vita")
spell:register()