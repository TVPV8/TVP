local combat = Combat()
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, false)
combat:setArea(createCombatArea(AREA_CIRCLE3X3))

local spell = Spell(SPELL_INSTANT)

function onTargetCreature(creature, target)
	if target ~= creature then
		if target:isPlayer() then
			if Game.getWorldType() == WORLD_TYPE_PVP_ENFORCED then
				local item = target:getSlotItem(CONST_SLOT_RING)
				if item and item:getId() == 2202 and math.random(1, 5) == 1 then
					item:remove()
				end
			end
		end
		target:removeCondition(CONDITION_INVISIBLE)
	end
	return true
end

combat:setCallback(CALLBACK_PARAM_TARGETCREATURE, "onTargetCreature")

function spell.onCastSpell(creature, variant)
	return combat:execute(creature, variant)
end

spell:needLearn(true)
spell:mana(200)
spell:magicLevel(12)
spell:isPremium(true)
spell:isAggressive(false)
spell:name("Cancel Invisibility")
spell:vocation("Sorcerer", "Master Sorcerer")
spell:words("ex,ana, ina")
spell:register()