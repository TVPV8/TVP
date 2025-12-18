local spell = Spell(SPELL_INSTANT)

spell:needLearn(true)
spell:mana(100)
spell:magicLevel(11)
spell:soul(0)
spell:isAggressive(false)
spell:name("Ultimate Healing Rune")
spell:vocation("Druid", "Elder Druid")
spell:words("ad,ura, vita")

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(100, 2260, 2273, 1)
end

spell:register()

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_HEALING)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
combat:setParameter(COMBAT_PARAM_DISPEL, CONDITION_PARALYZE)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, false)
combat:setParameter(COMBAT_PARAM_TARGETCASTERORTOPMOST, false)

function onGetFormulaValues(player, level, magicLevel)
	return player:computeHealing(250, 0, true)
end

combat:setCallback(CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local rune = Spell(SPELL_RUNE)

function rune.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

rune:runeMagicLevel(4)
rune:runeId(2273)
rune:charges(1)
rune:allowFarUse(true)
rune:needTarget(true)
rune:isAggressive(false)
rune:isBlocking(true)
rune:cooldownSpellTime(false) -- on 7.7 it allows for another spell cast
rune:register()
