local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(60)
spell:magicLevel(4)
spell:soul(0)
spell:isAggressive(false)
spell:name("Intense Healing Rune")
spell:vocation("Druid", "Elder Druid")
spell:words("ad,ura, gran")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(60, 2260, 2265, 1)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_HEALING)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
combat:setParameter(COMBAT_PARAM_DISPEL, CONDITION_PARALYZE)
combat:setParameter(COMBAT_PARAM_TARGETCASTERORTOPMOST, false)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, false)

function onGetFormulaValues(player, level, magicLevel)
	return player:computeHealing(70, 30, true)
end

combat:setCallback(CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

rune:runeMagicLevel(1)
rune:runeId(2265)
rune:charges(1)
rune:allowFarUse(true)
rune:blockWalls(true)
rune:checkFloor(true)
rune:needTarget(true)
rune:isAggressive(false)
rune:cooldownSpellTime(false) -- on 7.7 it allows for another spell cast
rune:register()
