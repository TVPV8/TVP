local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(200)
spell:magicLevel(13)
spell:soul(0)
spell:isAggressive(false)
spell:name("Fire Wall")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid")
spell:words("ad,evo, mas, grav, flam")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(200, 2260, 2303, 4)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_FIREDAMAGE)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_FIRE)
combat:setParameter(COMBAT_PARAM_CREATEITEM, ITEM_FIREFIELD_PVP_FULL)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setArea(createCombatArea(AREA_WALLFIELD_SMALL, AREADIAGONAL_WALLFIELD_SMALL))

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

rune:runeMagicLevel(6)
rune:runeId(2303)
rune:charges(4)
rune:allowFarUse(true)
rune:blockWalls(true)
rune:checkFloor(true)
rune:isBlocking(true)
rune:isPzLock(true)
rune:register()
