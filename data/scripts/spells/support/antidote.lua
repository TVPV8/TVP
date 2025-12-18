local combat = Combat()
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
combat:setParameter(COMBAT_PARAM_DISPEL, CONDITION_POISON)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, false)
combat:setParameter(COMBAT_PARAM_TARGETCASTERORTOPMOST, true)

local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

spell:isSelfTarget(true)
spell:needLearn(true)
spell:mana(30)
spell:magicLevel(2)
spell:isAggressive(false)
spell:name("Antidote")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid", "Paladin", "Royal Paladin", "Knight", "Elite Knight")
spell:words("ex,ana, pox")
spell:register()