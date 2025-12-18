local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_EARTHDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_GREEN_RINGS)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setParameter(COMBAT_PARAM_FORCEONTARGETEVENT, true)
combat:setArea(createCombatArea(AREA_CIRCLE6X6))

local condition = Condition(CONDITION_POISON)

local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	local blockPz = false
	local cycles = -creature:computeDamage(200, 50)
	condition:setParameter(CONDITION_PARAM_CYCLE, cycles)
	condition:setParameter(CONDITION_PARAM_COUNT, 3)
	condition:setParameter(CONDITION_PARAM_MAX_COUNT, 3)
	condition:setParameter(CONDITION_PARAM_OWNERGUID, creature:getGuid())
	for _, target in ipairs(combat:getTargets(creature, variant)) do
		if target:isPlayer() then
			blockPz = true
		end
		target:addCondition(condition)
	end

	if blockPz then
		creature:setInFight(true)
	end

	return true
end

spell:needLearn(true)
spell:mana(600)
spell:magicLevel(28)
spell:isPremium(true)
spell:isAggressive(true)
spell:name("Poison Storm")
spell:vocation("Druid", "Elder Druid")
spell:words("ex,evo, gran, mas, pox")
spell:register()