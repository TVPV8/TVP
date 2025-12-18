local combat = Combat()
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, false)
combat:setParameter(COMBAT_PARAM_TARGETCASTERORTOPMOST, true)

local condition = Condition(CONDITION_LIGHT)
condition:setParameter(CONDITION_PARAM_CYCLE, 6)
condition:setParameter(CONDITION_PARAM_COUNT, 500 / 6)
condition:setParameter(CONDITION_PARAM_MAX_COUNT, 500 / 6)
combat:addCondition(condition)

local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

spell:isSelfTarget(true)
spell:needLearn(true)
spell:mana(20)
spell:magicLevel(0)
spell:isAggressive(false)
spell:name("Light")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid", "Paladin", "Royal Paladin", "Knight", "Elite Knight")
spell:words("ut,evo, lux")
spell:register()