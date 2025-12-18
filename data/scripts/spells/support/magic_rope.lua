local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	local position = creature:getPosition()

	local tile = Tile(position)
	if not table.contains(ropeSpots, tile:getGround():getId()) then
		creature:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
		creature:getPosition():sendMagicEffect(CONST_ME_POFF)
		return false
	end

	tile = Tile(position:moveUpstairs())
	if not tile then
		creature:sendCancelMessage(RETURNVALUE_NOTENOUGHROOM)
		creature:getPosition():sendMagicEffect(CONST_ME_POFF)
		return false
	end

	creature:getPosition():sendMagicEffect(CONST_ME_TELEPORT)
	creature:teleportTo(position, false)
	return true
end

spell:isSelfTarget(true)
spell:needLearn(true)
spell:mana(20)
spell:magicLevel(1)
spell:isPremium(true)
spell:isAggressive(false)
spell:name("Magic Rope")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid", "Knight", "Elite Knight", "Paladin", "Royal Paladin")
spell:words("ex,ani, tera")
spell:register()
