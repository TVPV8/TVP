local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_HEALING)
combat:setParameter(COMBAT_PARAM_DISPEL, CONDITION_PARALYZE)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, false)

function onGetFormulaValues(player, level, magicLevel)
	return player:computeHealing(120, 40, true)
end

combat:setCallback(CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	local target = Creature(variant.number)
	local fromPos = creature:getPosition()
	local toPos = target:getPosition()
	
	if fromPos:getDistanceX(toPos) > 7 then
		creature:sendCancelMessage(RETURNVALUE_DESTINATIONOUTOFREACH)
		return false
	end
	
	if fromPos:getDistanceY(toPos) > 5 then
		creature:sendCancelMessage(RETURNVALUE_DESTINATIONOUTOFREACH)
		return false
	end
	
    local casterIsTarget = target == creature
    local magicEffect = casterIsTarget and CONST_ME_MAGIC_BLUE or CONST_ME_MAGIC_GREEN

    if combat:execute(creature, variant) then
        creature:getPosition():sendMagicEffect(CONST_ME_MAGIC_BLUE)
        if not casterIsTarget then
            target:getPosition():sendMagicEffect(magicEffect)
        end
        return true
    end

    return false
end

spell:needLearn(true)
spell:mana(70)
spell:magicLevel(7)
spell:isPremium(true)
spell:isAggressive(false)
spell:hasPlayerNameParam(true)
spell:hasParams(true)
spell:needTarget(true)
spell:isBlockingWalls(false)
spell:name("Heal Friend")
spell:vocation("Druid", "Elder Druid")
spell:words("ex,ura, sio")
spell:register()
