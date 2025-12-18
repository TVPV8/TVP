local combat = Combat()
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MORTAREA)
combat:setArea(createCombatArea(AREA_SQUARE1X1))

function onTargetCreature(creature, target)
	if target:isMonster() then
		target:addHealth(-target:getMaxHealth())
	end
end

combat:setCallback(CALLBACK_PARAM_TARGETCREATURE, "onTargetCreature")

local talkaction = TalkAction("/killall")

function talkaction.onSay(player, words, param, type)
	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	combat:execute(player, Variant(player:getPosition()))
	return false
end

talkaction:access(true)
talkaction:separator(" ")
talkaction:register()