local marks = {
	{"Thais", {x = 32369, y = 32215, z = 7}},
	{"Carlin", {x = 32341, y = 31789, z = 7}},
	{"Ab'Dendriel", {x = 32661, y = 31687, z = 7}},
	{"Rookgaard", {x = 32097, y = 32207, z = 7}},
	{"Fibula", {x = 32176, y = 32437, z = 7}},
	{"Kazordoon", {x = 32632, y = 31916, z = 8}},
	{"Senja", {x = 32125, y = 31667, z = 7}},
	{"Folda", {x = 32046, y = 31582, z = 7}},
	{"Vega", {x = 32027, y = 31692, z = 7}},
	{"Havoc", {x = 32783, y = 32243, z = 6}},
	{"Orc", {x = 32901, y = 31771, z = 7}},
	{"Minocity", {x = 32404, y = 32124, z = 15}},
	{"Minoroom", {x = 32139, y = 32109, z = 11}},
	{"Desert", {x = 32653, y = 32117, z = 7}},
	{"Swamp", {x = 32724, y = 31976, z = 6}},
	{"Home", {x = 32316, y = 31942, z = 7}},
	{"Mists", {x = 32854, y = 32333, z = 6}},
	{"FibulaDungeon", {x = 32189, y = 32426, z = 9}},
	{"DragonIsle", {x = 32781, y = 31603, z = 7}},
	{"HellsGate", {x = 32675, y = 31648, z = 10}},
	{"Necropolis", {x = 32786, y = 31683, z = 14}},
	{"Trollcaves", {x = 32493, y = 32259, z = 8}},
	{"Elvenbane", {x = 32590, y = 31657, z = 7}},
	{"Fieldofglory", {x = 32430, y = 31671, z = 7}},
	{"Hills", {x = 32553, y = 31827, z = 6}},
	{"Sternum", {x = 32463, y = 32077, z = 7}},
	{"Northport", {x = 32486, y = 31610, z = 7}},
	{"Greenshore", {x = 32273, y = 32053, z = 7}},
	{"Edron", {x = 33191, y = 31818, z = 7}},
	{"Stonehome", {x = 33319, y = 31766, z = 7}},
	{"Camp", {x = 32655, y = 32208, z = 7}},
	{"Cormaya", {x = 33302, y = 31970, z = 7}},
	{"Darashia", {x = 33224, y = 32428, z = 7}},
	{"Drefia", {x = 32996, y = 32417, z = 7}},
	{"Venore", {x = 32955, y = 32076, z = 6}},
	{"Ghostship", {x = 33325, y = 32173, z = 6}},
	{"VenoreDragons", {x = 32793, y = 32155, z = 8}},
	{"Shadowthorn", {x = 33086, y = 32157, z = 7}},
	{"Amazons", {x = 32839, y = 31925, z = 7}},
	{"KingsIsle", {x = 32174, y = 31940, z = 7}},
	{"Ghostlands", {x = 32223, y = 31831, z = 7}},
	{"Ankrahmun", {x = 33162, y = 32802, z = 7}},
	{"Oasis", {x = 33132, y = 32661, z = 7}},
	{"Marid", {x = 33103, y = 32539, z = 6}},
	{"Efreet", {x = 33053, y = 32622, z = 6}},
	{"PortHope", {x = 32623, y = 32753, z = 7}},
	{"Banuta", {x = 32812, y = 32559, z = 7}},
	{"Chor", {x = 32956, y = 32843, z = 7}},
	{"Trapwood", {x = 32709, y = 32901, z = 8}},
	{"Eremo", {x = 33323, y = 31883, z = 7}},
}

local condition = Condition(CONDITION_HASTE)
condition:setParameter(CONDITION_PARAM_TICKS, -1)

local talkaction = TalkAction("alani")

function talkaction.onSay(player, words, param, type)
	if param == "normal" or param == "slow" then
		player:removeCondition(CONDITION_HASTE)
		player:getPosition():sendMagicEffect(CONST_ME_MAGIC_BLUE)
	elseif param == "fast" then
		condition:setParameter(CONDITION_PARAM_SPEED, 100)
		player:addCondition(condition)
		player:getPosition():sendMagicEffect(CONST_ME_MAGIC_BLUE)
	elseif param == "fastest" then
		condition:setParameter(CONDITION_PARAM_SPEED, 200)
		player:addCondition(condition)
		player:getPosition():sendMagicEffect(CONST_ME_MAGIC_BLUE)
	else
		local floorchangeIndex = string.find(param, "hur")
		if floorchangeIndex ~= nil then
			local fromPosition = player:getPosition()

			if string.find(param, "up") and fromPosition.z ~= 8 or string.find(param, "down") and fromPosition.z ~= 7 then
				local toPosition = player:getPosition()
				toPosition:getNextPosition(player:getDirection())

				local tile = Tile(string.find(param, "up") and Position(fromPosition.x, fromPosition.y, fromPosition.z - 1) or toPosition)
				if not tile or not tile:getGround() and not tile:hasFlag(string.find(param, "up") and TILESTATE_IMMOVABLEBLOCKSOLID or TILESTATE_BLOCKSOLID) then
					tile = Tile(toPosition.x, toPosition.y, toPosition.z + (string.find(param, "up") and -1 or 1))

					if tile and tile:getGround() and not tile:hasFlag(bit.bor(TILESTATE_IMMOVABLEBLOCKSOLID, TILESTATE_FLOORCHANGE)) then
						player:getPosition():sendMagicEffect(CONST_ME_TELEPORT)
						return player:move(tile, bit.bor(FLAG_IGNOREBLOCKITEM, FLAG_IGNOREBLOCKCREATURE))
					end
				end
				player:getPosition():sendMagicEffect(CONST_ME_POFF)
				return false
			end
		end

		for _, i in ipairs(marks) do
			if i[1]:lower() == param then
				player:teleportTo(i[2])
				player:getPosition():sendMagicEffect(CONST_ME_TELEPORT)
				return false
			end
		end

		player:sendCancelMessage("There is no mark of this name.")
	end
	return false
end

talkaction:access(true)
talkaction:separator(" ")
talkaction:register()