local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(spell, 0, 2545, 10)
end

spell:needLearn(true)
spell:mana(70)
spell:magicLevel(5)
spell:soul(0)
spell:isAggressive(false)
spell:name("Poisoned Arrow")
spell:vocation("Paladin", "Royal Paladin")
spell:words("ex,evo, con, pox")
spell:register()