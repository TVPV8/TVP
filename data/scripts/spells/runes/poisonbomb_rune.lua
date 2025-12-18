local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(130)
spell:magicLevel(8)
spell:soul(0)
spell:isAggressive(false)
spell:isPremium(true)
spell:name("Poisonbomb")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid")
spell:words("ad,evo, mas, pox")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(130, 2260, 2286, 2)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_EARTHDAMAGE)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_POISON)
combat:setParameter(COMBAT_PARAM_CREATEITEM, ITEM_POISONFIELD_PVP)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setArea(createCombatArea(AREA_SQUARE1X1))

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	return combat:execute(creature, variant, false)
end

rune:runeMagicLevel(4)
rune:runeId(2286)
rune:charges(2)
rune:allowFarUse(true)
rune:checkFloor(true)
rune:isBlocking(true)
rune:isPzLock(true)
rune:register()