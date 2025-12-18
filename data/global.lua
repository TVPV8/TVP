math.randomseed(os.time())

dofile('data/lib/lib.lua')

actionIds = {
	puzzleSwitch = 4000, -- a switch that cannot be moved back and will decay into its original ID (puzzle lever)
	sandstoneWall = 4001, -- a sandstone wall that is walkable
	sandHole = 4002, -- hidden sand hole
	pickHole = 4003, -- hidden mud hole
	destroyableStone = 4004, -- stone that is destroyable with a pick on the map
	blockingTile = 4005, -- does not allow any creature to walk through it
}

-- All corpses (human corpses), used mostly for desintegrate rune
corpseIds = {
	3058, 3059, 3060, 3061, 3064, 3065, 3066
}

-- This array contains all destroyable field items
Fields = {
	1487, 1488, 1489, 1490, 1491, 1492, 1493, 1494, 
	1495, 1496, 1500, 1501, 1502, 1503, 1504, 1505
}

ropeSpots = {
	384, 418
}

keys = {
	2086, 2087, 2088, 2089, 2090, 2091, 2092
}

openDoors = {
	1211, 1214, 1233, 1236, 1251, 1254, 3546, 3537, 4915, 4918
}

closedDoors = {
	1210, 1213, 1232, 1235, 1250, 1253, 3536, 3545, 4913, 4916
}

lockedDoors = {
	1209, 1212, 1231, 1234, 1249, 1252, 3535, 3544, 4914, 4917
}

openExtraDoors = {
	1540, 1542
}
closedExtraDoors = {
	1539, 1541
}

openHouseDoors = {
	1220, 1222, 1238, 1240, 3539, 3548
}
closedHouseDoors = {
	1219, 1221, 1237, 1239, 3538, 3547
}

openQuestDoors = {
	1224, 1226, 1242, 1244, 1256, 1258, 3543, 3552
}
closedQuestDoors = {
	1223, 1225, 1241, 1243, 1255, 1257, 3542, 3551
}

openLevelDoors = {
	1228, 1230, 1246, 1248, 1260, 1262, 3541, 3550
}
closedLevelDoors = {
	1227, 1229, 1245, 1247, 1259, 1261, 3540, 3549
}

jungleGrass = { -- grass destroyable by machete
	[2782] = 2781,
	[3985] = 3984,
}

pickGrounds = {354, 355} -- pick usable ground

sandIds = {231} -- desert sand for shovel (scarab coins, scarab spawn)

holeId = { -- usable rope holes (for roping creatures/items from below)
	294, 369, 370, 383, 392, 408, 409, 410, 427, 428, 429, 430, 462, 469, 470, 482,
	484, 485, 489, 924, 3135, 3136, 3311, 3324, 4835, 4837,
}

holes = {468, 481, 483} -- holes opened by shovel

function getDistanceBetween(firstPosition, secondPosition)
	local xDif = math.abs(firstPosition.x - secondPosition.x)
	local yDif = math.abs(firstPosition.y - secondPosition.y)
	local posDif = math.max(xDif, yDif)
	if firstPosition.z ~= secondPosition.z then
		posDif = posDif + 15
	end
	return posDif
end

function getFormattedWorldTime()
	local worldTime = getWorldTime()
	local hours = math.floor(worldTime / 60)

	local minutes = worldTime % 60
	if minutes < 10 then
		minutes = '0' .. minutes
	end
	return hours .. ':' .. minutes
end

function getLootRandom()
	return math.random(0, MAX_LOOTCHANCE) / configManager.getNumber(configKeys.RATE_LOOT)
end

table.contains = function(array, value)
	for _, targetColumn in pairs(array) do
		if targetColumn == value then
			return true
		end
	end
	return false
end

string.split = function(str, sep)
	local res = {}
	for v in str:gmatch("([^" .. sep .. "]+)") do
		res[#res + 1] = v
	end
	return res
end

string.splitTrimmed = function(str, sep)
	local res = {}
	for v in str:gmatch("([^" .. sep .. "]+)") do
		res[#res + 1] = v:trim()
	end
	return res
end

string.trim = function(str)
	return str:match'^()%s*$' and '' or str:match'^%s*(.*%S)'
end

if not nextUseStaminaTime then
	nextUseStaminaTime = {}
end

function getPlayerDatabaseInfo(name_or_guid)
	local sql_where = ""

	if type(name_or_guid) == 'string' then
		sql_where = "WHERE `p`.`name`=" .. db.escapeString(name_or_guid) .. ""
	elseif type(name_or_guid) == 'number' then
		sql_where = "WHERE `p`.`id`='" .. name_or_guid .. "'"
	else
		return false
	end

	local sql_query = [[
		SELECT
			`p`.`id` as `guid`,
			`p`.`name`,
			CASE WHEN `po`.`player_id` IS NULL
				THEN 0
				ELSE 1
			END AS `online`,
			`p`.`group_id`,
			`p`.`level`,
			`p`.`experience`,
			`p`.`vocation`,
			`p`.`maglevel`,
			`p`.`skill_fist`,
			`p`.`skill_club`,
			`p`.`skill_sword`,
			`p`.`skill_axe`,
			`p`.`skill_dist`,
			`p`.`skill_shielding`,
			`p`.`skill_fishing`,
			`p`.`town_id`,
			`p`.`balance`,
			`gm`.`guild_id`,
			`gm`.`nick`,
			`g`.`name` AS `guild_name`,
			CASE WHEN `p`.`id` = `g`.`ownerid`
				THEN 1
				ELSE 0
			END AS `is_leader`,
			`gr`.`name` AS `rank_name`,
			`gr`.`level` AS `rank_level`,
			`h`.`id` AS `house_id`,
			`h`.`name` AS `house_name`,
			`h`.`town_id` AS `house_town`
		FROM `players` AS `p`
		LEFT JOIN `players_online` AS `po`
			ON `p`.`id` = `po`.`player_id`
		LEFT JOIN `guild_membership` AS `gm`
			ON `p`.`id` = `gm`.`player_id`
		LEFT JOIN `guilds` AS `g`
			ON `gm`.`guild_id` = `g`.`id`
		LEFT JOIN `guild_ranks` AS `gr`
			ON `gm`.`rank_id` = `gr`.`id`
		LEFT JOIN `houses` AS `h`
			ON `p`.`id` = `h`.`owner`
	]] .. sql_where

	local query = db.storeQuery(sql_query)
	if not query then
		return false
	end

	local info = {
		["guid"] = result.getNumber(query, "guid"),
		["name"] = result.getString(query, "name"),
		["online"] = result.getNumber(query, "online"),
		["group_id"] = result.getNumber(query, "group_id"),
		["level"] = result.getNumber(query, "level"),
		["experience"] = result.getNumber(query, "experience"),
		["vocation"] = result.getNumber(query, "vocation"),
		["maglevel"] = result.getNumber(query, "maglevel"),
		["skill_fist"] = result.getNumber(query, "skill_fist"),
		["skill_club"] = result.getNumber(query, "skill_club"),
		["skill_sword"] = result.getNumber(query, "skill_sword"),
		["skill_axe"] = result.getNumber(query, "skill_axe"),
		["skill_dist"] = result.getNumber(query, "skill_dist"),
		["skill_shielding"] = result.getNumber(query, "skill_shielding"),
		["skill_fishing"] = result.getNumber(query, "skill_fishing"),
		["town_id"] = result.getNumber(query, "town_id"),
		["balance"] = result.getNumber(query, "balance"),
		["guild_id"] = result.getNumber(query, "guild_id"),
		["nick"] = result.getString(query, "nick"),
		["guild_name"] = result.getString(query, "guild_name"),
		["is_leader"] = result.getNumber(query, "is_leader"),
		["rank_name"] = result.getString(query, "rank_name"),
		["rank_level"] = result.getNumber(query, "rank_level"),
		["house_id"] = result.getNumber(query, "house_id"),
		["house_name"] = result.getString(query, "house_name"),
		["house_town"] = result.getNumber(query, "house_town")
	}

	result.free(query)
	return info
end
