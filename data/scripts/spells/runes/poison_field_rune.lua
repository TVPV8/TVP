local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(50)
spell:magicLevel(2)
spell:soul(0)
spell:isAggressive(false)
spell:name("Poison Field")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid")
spell:words("ad,evo, grav, pox")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(50, 2260, 2285, 3)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_EARTHDAMAGE)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_POISON)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setParameter(COMBAT_PARAM_CREATEITEM, ITEM_POISONFIELD_PVP)

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	local tile = Tile(variant.pos)
	if tile then
		if tile:hasProperty(CONST_PROP_BLOCKSOLID) then
			creature:sendCancelMessage(RETURNVALUE_NOTENOUGHROOM)
			return false
		end
	end
	return combat:execute(creature, variant)
end

rune:runeMagicLevel(0)
rune:runeId(2285)
rune:charges(3)
rune:allowFarUse(true)
rune:blockWalls(true)
rune:checkFloor(true)
rune:isBlocking(true)
rune:isPzLock(true)
rune:register()
