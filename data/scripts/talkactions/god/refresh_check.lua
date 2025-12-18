local talkaction = TalkAction("/refreshcheck")

function talkaction.onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	local range = tonumber(param) or 1

	local positions = {}
	local area = Combat:getCircleArea(player, range)

	for _, tile in pairs(area) do
		if tile:hasFlag(TILESTATE_REFRESH) then
			tile:getPosition():sendMagicEffect(11)
		end
	end

	return false
end

talkaction:separator(" ")
talkaction:register()

local talkaction = TalkAction("/norefreshcheck")

function talkaction.onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	local range = tonumber(param) or 1

	local positions = {}
	local area = Combat:getCircleArea(player, range, false)

	for _, tile in pairs(area) do
		if not tile:hasFlag(TILESTATE_REFRESH) then
			tile:getPosition():sendMagicEffect(CONST_ME_TELEPORT)
		end
	end

	return false
end

talkaction:separator(" ")
talkaction:register()