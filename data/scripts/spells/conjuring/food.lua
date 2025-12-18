local foods = {
	2666, -- meat
	2671, -- ham
	2681, -- grape
	2674, -- apple
	2689, -- bread
	2690, -- roll
	2696  -- cheese
}

local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	if math.random(0, 1) == 1 then
		creature:addItem(foods[math.random(#foods)])
	end

	creature:addItem(foods[math.random(#foods)])
	creature:getPosition():sendMagicEffect(CONST_ME_MAGIC_GREEN)
	return true
end

spell:needLearn(true)
spell:mana(30)
spell:magicLevel(0)
spell:isAggressive(false)
spell:name("Food")
spell:vocation("Druid", "Elder Druid", "Paladin", "Royal Paladin")
spell:words("ex,evo, pan")
spell:register()