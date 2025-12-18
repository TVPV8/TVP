local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	return creature:conjureItem(spell, 2401, 2433, 1)
end

spell:needLearn(true)
spell:mana(80)
spell:magicLevel(22)
spell:soul(0)
spell:isPremium(true)
spell:isAggressive(false)
spell:name("Enchant Staff")
spell:vocation("Master Sorcerer")
spell:words("ex,eta, vis")
spell:register()