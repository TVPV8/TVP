local talkaction = TalkAction("/unlockaccount")

function talkaction.onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	if tonumber(param) <= 0 or tonumber(param) == nil then
		player:sendCancelMessage("Type a valid accountnumber.")
		return false
	end
	
	Game.unlockAccount(tonumber(param))
	return false
end

talkaction:separator(" ")
talkaction:register()