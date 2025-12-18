local condition = Condition(CONDITION_OUTFIT)
condition:setTicks(200000)

local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	local returnValue = RETURNVALUE_NOERROR
	local monsterType = MonsterType(variant:getString())
	if not monsterType then
		returnValue = RETURNVALUE_CREATUREDOESNOTEXIST
	elseif not creature:hasFlag(PlayerFlag_CanIllusionAll) and not monsterType:isIllusionable() then
		returnValue = RETURNVALUE_NOTPOSSIBLE
	end

	if returnValue ~= RETURNVALUE_NOERROR then
		creature:sendCancelMessage(returnValue)
		creature:getPosition():sendMagicEffect(CONST_ME_POFF)
		return false
	end

	condition:setOutfit(monsterType:getOutfit())
	creature:addCondition(condition)
	creature:getPosition():sendMagicEffect(CONST_ME_MAGIC_BLUE)
	return true
end

spell:needLearn(true)
spell:mana(100)
spell:magicLevel(10)
spell:isPremium(true)
spell:isAggressive(false)
spell:hasParams(true)
spell:name("Creature Illusion")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid")
spell:words("ut,evo, res, ina")
spell:register()