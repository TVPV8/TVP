local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(150)
spell:magicLevel(13)
spell:soul(0)
spell:isPremium(true)
spell:isAggressive(false)
spell:name("Soulfire")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid")
spell:words("ad,evo, res, flam")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(150, 2260, 2308, 2)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_FIREDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_HITBYFIRE)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_FIRE)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setParameter(COMBAT_PARAM_NODAMAGE, true)

local condition = Condition(CONDITION_FIRE)

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	local cycles = -creature:computeDamage(120, 20)
	condition:setParameter(CONDITION_PARAM_CYCLE, cycles / 10)
	condition:setParameter(CONDITION_PARAM_COUNT, 8)
	condition:setParameter(CONDITION_PARAM_MAX_COUNT, 8)
	condition:setParameter(CONDITION_PARAM_OWNERGUID, creature:getGuid())
	local target = Creature(variant.number)
	if target then
		if target ~= creature then
			target:addCondition(condition)
		end
	end
	return combat:execute(creature, variant)
end

rune:runeMagicLevel(7)
rune:runeId(2308)
rune:charges(2)
rune:allowFarUse(true)
rune:blockWalls(true)
rune:checkFloor(true)
rune:needTarget(true)
rune:register()
