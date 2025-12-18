local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(160)
spell:magicLevel(11)
spell:soul(0)
spell:isAggressive(false)
spell:name("Poison Wall")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid")
spell:words("ad,evo, mas, grav, pox")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(160, 2260, 2289, 4)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_EARTHDAMAGE)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_POISON)
combat:setParameter(COMBAT_PARAM_CREATEITEM, ITEM_POISONFIELD_PVP)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setArea(createCombatArea(AREA_WALLFIELD_SMALL, AREADIAGONAL_WALLFIELD_SMALL))

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

rune:runeMagicLevel(5)
rune:runeId(2289)
rune:charges(4)
rune:allowFarUse(true)
rune:blockWalls(true)
rune:checkFloor(true)
rune:isBlocking(true)
rune:isPzLock(true)
rune:register()
