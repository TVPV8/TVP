local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(60)
spell:magicLevel(3)
spell:soul(0)
spell:isAggressive(false)
spell:name("Fire Field")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid")
spell:words("ad,evo, grav, flam")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(60, 2260, 2301, 3)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_FIREDAMAGE)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_FIRE)
combat:setParameter(COMBAT_PARAM_CREATEITEM, ITEM_FIREFIELD_PVP_FULL)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)

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

rune:runeMagicLevel(1)
rune:runeId(2301)
rune:charges(3)
rune:allowFarUse(true)
rune:blockWalls(true)
rune:checkFloor(true)
rune:isBlocking(true)
rune:isPzLock(true)
rune:register()
