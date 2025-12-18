local weapon = Weapon(WEAPON_AMMO)

local area = createCombatArea({
	{1, 1, 1},
	{1, 3, 1},
	{1, 1, 1}
})

local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_FIREAREA)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_BURSTARROW)
combat:setParameter(COMBAT_PARAM_BLOCKARMOR, true)
combat:setParameter(COMBAT_PARAM_BLOCKSHIELD, false)
combat:setArea(area)

function onGetFormulaValues(player, level, magicLevel)
	return player:computeDamage(30, 30)
end

combat:setCallback(CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

function weapon.onUseWeapon(player, variant, hit)
	local result = combat:execute(player, variant)
	if variant.number ~= nil then
		Creature(variant.number):getPosition():sendMagicEffect(CONST_ME_POFF)
	else
		variant.pos:sendMagicEffect(CONST_ME_POFF)
	end
	return result
end

weapon:action("removecount")
weapon:id(2546)
weapon:register()
