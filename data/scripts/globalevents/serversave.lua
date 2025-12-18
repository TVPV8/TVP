local config = {
	saveTime = 5 * 60 * 1000,
	closeServer = true, -- closes when preparing to save (within save time)
	shutdownServer = true, -- shutsdown the server
}

local globalevent = GlobalEvent("ServerSave")

function globalevent.onTime(interval)
    Game.broadcastMessage("Server is saving game in " .. config.saveTime / 60000 .. " minutes.\nPlease come back in 10 minutes.", MESSAGE_STATUS_WARNING)

    if config.closeServer then
		-- Do not allow players to login until server has restarted
        Game.setGameState(GAME_STATE_CLOSED)
    end

	-- First warning in 2 minutes
    addEvent(function () 
		Game.broadcastMessage("Server is saving game in 3 minutes.\nPlease come back in 10 minutes.", MESSAGE_STATUS_WARNING)
	end, config.saveTime - 3 * 60 * 1000)

	-- Last warning in 4 minutes
	addEvent(function () 
		Game.broadcastMessage("Server is saving game in one minute.\nPlease log out.", MESSAGE_STATUS_WARNING)
	end, config.saveTime - 1 * 60 * 1000)

	-- Server save function
	addEvent(function () 
		if config.shutdownServer then
			Game.setGameState(GAME_STATE_SHUTDOWN)
		else
			-- Save server without shutdown
			saveServer()
		end
	end, config.saveTime)

    return true
end

globalevent:time(configManager.getString(configKeys.SERVER_SAVE_TIME))
globalevent:register()