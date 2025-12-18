function canJoin(player)
	return player:getAccountType() >= ACCOUNT_TYPE_GAMEMASTER
end

function onSpeak(player, type, message)
	return type
end
