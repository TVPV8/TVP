local combat = Combat()
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_ENERGY)
combat:setParameter(COMBAT_PARAM_CREATEITEM, 1499)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, true)

local spell = Spell(SPELL_INSTANT)

function spell.onCastSpell(creature, variant)
	local position = creature:getPosition()
	position:getNextPosition(creature:getDirection())
	local tile = Tile(position)
	if tile and tile:hasFlag(TILESTATE_PROTECTIONZONE) then
		creature:sendCancelMessage(RETURNVALUE_ACTIONNOTPERMITTEDINPROTECTIONZONE)
		creature:getPosition():sendMagicEffect(CONST_ME_POFF)
		return false
	end
	return combat:execute(creature, variant)
end

spell:needLearn(true)
spell:mana(220)
spell:magicLevel(13)
spell:soul(0)
spell:isPremium(true)
spell:isAggressive(true)
spell:needDirection(true)
spell:isBlocking(true, true)
spell:name("Wild Growth")
spell:vocation("Elder Druid")
spell:words("ex,evo, grav, vita")
spell:register()
