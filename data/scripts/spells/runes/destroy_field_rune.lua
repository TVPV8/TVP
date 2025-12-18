local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(60)
spell:magicLevel(6)
spell:soul(0)
spell:isAggressive(false)
spell:name("Destroy Field")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid", "Paladin", "Royal Paladin")
spell:words("ad,ito, grav")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(60, 2260, 2261, 3)
end

spell:register()

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	local position = variant:getPosition()
	local tile = Tile(position)
	local field = tile and tile:getItemByType(ITEM_TYPE_MAGICFIELD)
	if field and table.contains(Fields, field:getId()) then
		field:remove()
		position:sendMagicEffect(CONST_ME_POFF)
		return true
	end

	creature:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
	creature:getPosition():sendMagicEffect(CONST_ME_POFF)
	return false
end

rune:runeMagicLevel(3)
rune:runeId(2261)
rune:charges(3)
rune:allowFarUse(true)
rune:blockWalls(true)
rune:checkFloor(true)
rune:isBlocking(true)
rune:isAggressive(true)
rune:register()
