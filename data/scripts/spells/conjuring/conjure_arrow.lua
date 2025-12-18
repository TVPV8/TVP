local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(spell, 0, 2544, 15)
end

spell:needLearn(true)
spell:mana(40)
spell:magicLevel(2)
spell:soul(0)
spell:isAggressive(false)
spell:name("Conjure Arrow")
spell:vocation("Paladin", "Royal Paladin")
spell:words("ex,evo, con")
spell:register()