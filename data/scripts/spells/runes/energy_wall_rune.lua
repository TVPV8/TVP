local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(250)
spell:magicLevel(18)
spell:soul(0)
spell:isPremium(true)
spell:isAggressive(false)
spell:name("Energy Wall")
spell:vocation("Sorcerer", "Master Sorcerer", "Druid", "Elder Druid")
spell:words("ad,evo, mas, grav, vis")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(250, 2260, 2279, 4)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_ENERGYDAMAGE)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_ENERGY)
combat:setParameter(COMBAT_PARAM_CREATEITEM, ITEM_ENERGYFIELD_PVP)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)
combat:setArea(createCombatArea(AREA_WALLFIELD, AREADIAGONAL_WALLFIELD))

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

rune:runeMagicLevel(9)
rune:runeId(2279)
rune:charges(4)
rune:allowFarUse(true)
rune:blockWalls(true)
rune:checkFloor(true)
rune:isBlocking(true)
rune:isPzLock(true)
rune:register()
