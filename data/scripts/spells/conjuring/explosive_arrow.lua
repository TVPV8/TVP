local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(spell, 0, 2546, 5)
end

spell:needLearn(true)
spell:mana(120)
spell:magicLevel(10)
spell:soul(0)
spell:isAggressive(false)
spell:isPremium(true)
spell:name("Explosive Arrow")
spell:vocation("Paladin", "Royal Paladin")
spell:words("ex,evo, con, flam")
spell:register()