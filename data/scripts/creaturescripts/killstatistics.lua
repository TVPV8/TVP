if not KILL_STATISTICS then
	KILL_STATISTICS = {}
end

local serverStartTime = os.time()

local creatureevent = CreatureEvent("KillStatistics_KillMonster")

-- Store information for player killing monsters
function creatureevent.onKill(creature, target)
	if target:getMaster() then
		return true -- Killed creature is a summon
	end

	local name = target:getName()
	if target:getPlayer() then
		name = "players" -- Killed creature is a player
	end

	if name == "Demon" then -- Special check for Demon "Illusion"
		local maxhp = target:getMaxHealth()
		if maxhp <= 50 then 
			name = "Illusion"
		end
	end

	local record = KILL_STATISTICS[name]
	if record then
		KILL_STATISTICS[name].killed = KILL_STATISTICS[name].killed + 1 -- Increase existing record entry
	else 
		KILL_STATISTICS[name] = {killed = 1, killedBy = 0} -- Add new record entry
	end

	return true
end

creatureevent:register()

creatureevent = CreatureEvent("KillStatistics_KillPlayer")

-- Store information for monsters killing players
function creatureevent.onDeath(creature, corpse, killer, mostDamageKiller, lastHitUnjustified, mostDamageUnjustified)
	local name = ""

	if not killer then
		name = "elemental forces" -- If killer not found, player died from conditions
	elseif killer:getPlayer() then
		name = "players" -- Killer was a player
	elseif killer:getMaster() then
		local master = killer:getMaster()
		if master:getPlayer() then
			name = "players" -- Killer was a player summon
		else
			name = master:getName() -- Killer was a monster summon
		end
	else
		name = killer:getName() -- Killer was a normal monster
	end

	if name == "Demon" then -- Special check for Demon "Illusion"
		local maxhp = killer:getMaxHealth()
		if maxhp <= 50 then 
			name = "Illusion"
		end
	end

	local record = KILL_STATISTICS[name]
	if record then
		KILL_STATISTICS[name].killedBy = KILL_STATISTICS[name].killedBy + 1 -- Increase existing record entry
	else
		KILL_STATISTICS[name] = {killed = 0, killedBy = 1} -- Add new record entry
	end

	return true
end

creatureevent:register()

creatureevent = CreatureEvent("KillStatistics_PlayerLogin")

function creatureevent.onLogin(player)
    player:registerEvent("KillStatistics_KillMonster")
    player:registerEvent("KillStatistics_KillPlayer")
    return true
end

creatureevent:register()

local globalevent = GlobalEvent("KillStatistics_Flush")

function globalevent.onShutdown()
	print("> Flushing kill statistics ...")

	local dbQuery = "INSERT INTO `kill_statistics` (`name`, `killed_by`, `killed`, `time`) VALUES "

    local executeQuery = false
	for key, value in pairs(KILL_STATISTICS) do
        executeQuery = true
		dbQuery = dbQuery .. "(" .. db.escapeString(key) .. ", " .. value.killedBy .. ", " .. value.killed .. ", " .. serverStartTime .. "),"
	end

	if executeQuery then
		dbQuery = dbQuery:sub(1, #dbQuery - 1) -- Remove trailing commas
		db.query(dbQuery) -- Add data to database
	end

	return true
end

globalevent:register()