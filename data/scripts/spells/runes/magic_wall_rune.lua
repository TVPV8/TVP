local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(250)
spell:magicLevel(14)
spell:soul(0)
spell:isAggressive(false)
spell:isPremium(true)
spell:name("Magic Wall")
spell:vocation("Sorcerer", "Master Sorcerer")
spell:words("ad,evo, grav, tera")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(250, 2260, 2293, 4)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_ENERGY)
combat:setParameter(COMBAT_PARAM_CREATEITEM, ITEM_MAGICWALL)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	if variant.pos then
		local tile = Tile(variant.pos)
		if tile:hasProperty(CONST_PROP_BLOCKSOLID) then
			creature:sendTextMessage(MESSAGE_STATUS_SMALL, Game.getReturnMessage(RETURNVALUE_NOTENOUGHROOM))
			return false
		end
	end
	return combat:execute(creature, variant)
end

rune:runeMagicLevel(9)
rune:runeId(2293)
rune:charges(4)
rune:allowFarUse(true)
rune:checkFloor(true)
rune:isBlocking(true, true)
rune:isAggressive(true)
rune:register()
