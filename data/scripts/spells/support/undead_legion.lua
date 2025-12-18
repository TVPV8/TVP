local combat = Combat()
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
combat:setArea(createCombatArea(AREA_CIRCLE3X3))
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, false)

local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)	
	local positions = combat:getPositions(creature, Variant(creature:getPosition()))
    for i = 1, #positions do
		local position =  positions[i]
		local tile = Tile(position)
		if tile then
			local corpse = tile:getTopDownItem()
			if corpse then
				local itemType = corpse:getType()
				if itemType:isCorpse() and itemType:isMovable() and corpse:getActionId() == 0 then
					local monster = Game.createMonster("Skeleton", position)
					if monster then
						corpse:remove()
						creature:addSummon(monster)
					end
				end
			end
		end
    end
	return combat:execute(creature, variant)
end

spell:needLearn(true)
spell:mana(500)
spell:magicLevel(15)
spell:isPremium(true)
spell:isAggressive(false)
spell:name("Undead Legion")
spell:vocation("Druid", "Elder Druid")
spell:words("ex,ana, mas, mort")
spell:register()