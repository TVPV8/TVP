local talkaction = TalkAction("!share","/share")

function talkaction.onSay(player, words, param)
	local party = player:getParty()
	if not party then
		player:sendCancelMessage("You are not part of a party.")
		return false
	end
	
	if party:getLeader() ~= player then
		player:sendCancelMessage("You are not the leader of the party.")
		return false
	end
	
	if party:isSharedExperienceActive() then
		if player:getCondition(CONDITION_INFIGHT) then
			player:sendCancelMessage("You are in fight. Experience sharing has not been disabled.")
		else
			party:setSharedExperience(false)
		end
	else
		if player:getCondition(CONDITION_INFIGHT) then
			player:sendCancelMessage("You are in fight. Experience sharing has not been enabled.")
		else
			party:setSharedExperience(true)
		end
	end
		
	return false
end

talkaction:separator(" ")
talkaction:register()