local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(150)
spell:magicLevel(9)
spell:soul(0)
spell:isAggressive(false)
spell:name("Firebomb")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid")
spell:words("ad,evo, mas, flam")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(150, 2260, 2305, 2)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_FIREDAMAGE)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_FIRE)
combat:setParameter(COMBAT_PARAM_CREATEITEM, ITEM_FIREFIELD_PVP_FULL)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setArea(createCombatArea(AREA_SQUARE1X1))

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	return combat:execute(creature, variant, false)
end

rune:runeMagicLevel(5)
rune:runeId(2305)
rune:charges(2)
rune:allowFarUse(true)
rune:checkFloor(true)
rune:isBlocking(true)
rune:isPzLock(true)
rune:register()