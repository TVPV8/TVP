local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(spell, 0, 2547, 1)
end

spell:needLearn(true)
spell:mana(200)
spell:magicLevel(14)
spell:soul(0)
spell:isPremium(true)
spell:isAggressive(false)
spell:name("Conjure Power Bolt")
spell:vocation("Royal Paladin")
spell:words("ex,evo, con, vis")
spell:register()