local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(50)
spell:magicLevel(4)
spell:soul(0)
spell:isAggressive(false)
spell:name("Antidote Rune")
spell:vocation("Druid", "Elder Druid")
spell:words("ad,ana, pox")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(50, 2260, 2266, 1)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
combat:setParameter(COMBAT_PARAM_DISPEL, CONDITION_POISON)
combat:setParameter(COMBAT_PARAM_TARGETCASTERORTOPMOST, true)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, false)

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

rune:runeMagicLevel(0)
rune:runeId(2266)
rune:charges(1)
rune:allowFarUse(true)
rune:blockWalls(true)
rune:isAggressive(false)
rune:needTarget(true)
rune:register()
