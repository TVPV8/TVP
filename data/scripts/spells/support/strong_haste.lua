local combat = Combat()
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_GREEN)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, false)
combat:setParameter(COMBAT_PARAM_TARGETCASTERORTOPMOST, true)

local condition = Condition(CONDITION_HASTE)
condition:setParameter(CONDITION_PARAM_TICKS, 42500)
condition:setParameter(CONDITION_PARAM_SPEED, 70)
combat:addCondition(condition)

local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

spell:isSelfTarget(true)
spell:needLearn(true)
spell:mana(100)
spell:magicLevel(8)
spell:isPremium(true)
spell:isAggressive(false)
spell:name("Strong Haste")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid")
spell:words("ut,ani, gran, hur")
spell:register()