local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(100)
spell:magicLevel(8)
spell:soul(0)
spell:isAggressive(false)
spell:name("Desintegrate")
spell:vocation("Druid", "Elder Druid", "Paladin", "Royal Paladin", "Sorcerer", "Master Sorcerer")
spell:words("ad,ito, tera")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(100, 2260, 2310, 3)
end

spell:register()

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	local position = variant:getPosition()
	local tile = Tile(position)
	if tile then
		local items = tile:getItems()
		if items then
			for i, item in ipairs(items) do
				if item:getType():isMovable() and item:getUniqueId() > 65535 and item:getActionId() == 0 and not table.contains(corpseIds, item:getId()) then
					item:remove()
				end

				if i == 500 then
					break
				end
			end
		end
	end

	position:sendMagicEffect(CONST_ME_POFF)
	return true
end

rune:runeMagicLevel(4)
rune:runeId(2310)
rune:charges(3)
rune:allowFarUse(false)
rune:range(1)
rune:checkFloor(true)
rune:isBlocking(true)
rune:cooldownSpellTime(false)
rune:isAggressive(true)
rune:register()
