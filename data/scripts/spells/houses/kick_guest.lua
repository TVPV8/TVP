local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	local target = Player(variant:getString()) or creature
	local house = target:getTile():getHouse()
	if not house or not house:kickPlayer(creature, target) and target ~= creature then
		creature:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
		creature:getPosition():sendMagicEffect(CONST_ME_POFF)
		return false
	end
	target:teleportTo(house:getExitPosition())
	target:getPosition():sendMagicEffect(CONST_ME_TELEPORT)
	return true
end

spell:needLearn(false)
spell:hasParams(true)
spell:hasPlayerNameParam(true)
spell:isAggressive(false)
spell:words("alana sio")
spell:register()