local ec = EventCallback

ec.onDropLoot = function(self, corpse)
	if configManager.getNumber(configKeys.RATE_LOOT) == -1 then
		return
	end

	local player = Player(corpse:getCorpseOwner())
	local mType = self:getType()
	if not player or player:getStamina() > 840 then
		if not configManager.getBoolean(configKeys.MONSTERS_SPAWN_WITH_LOOT) then
			local lootBag = Game.createItem(1987, 1)
			corpse:addItemEx(lootBag)
			
			local monsterLoot = mType:getLoot()
			
			for i = 1, #monsterLoot do
				local itemType = ItemType(monsterLoot[i].itemId)
				if itemType:getWeaponType() == WEAPON_SHIELD or itemType:getWeaponType() == WEAPON_WAND or 
					itemType:getWeaponType() == WEAPON_DISTANCE or itemType:getDuration() > 0 or 
					itemType:getCharges() > 0 or itemType:hasStopDuration() then
					local item = lootBag:createLootItem(monsterLoot[i])
					if not item then
						print('[Warning] DropLoot:', 'Could not add loot item to loot bag.')
					end
				else
					local item = corpse:createLootItem(monsterLoot[i])
					if not item then
						print('[Warning] DropLoot:', 'Could not add loot item to corpse.')
					end
				end
			end
			
			if lootBag:getSize() == 0 then
				lootBag:remove()
			end
		end

		if configManager.getBoolean(configKeys.SHOW_MONSTER_LOOT_MESSAGE) then
			if player then
				local text = ("Loot of %s: %s"):format(mType:getNameDescription(), corpse:getContentDescription())
				local party = player:getParty()
				if party then
					party:broadcastPartyLoot(text)
				else
					player:sendTextMessage(MESSAGE_INFO_DESCR, text)
				end
			end
		end
	elseif configManager.getBoolean(configKeys.SHOW_MONSTER_LOOT_MESSAGE) then
		local text = ("Loot of %s: nothing (due to low stamina)"):format(mType:getNameDescription())
		local party = player:getParty()
		if party then
			party:broadcastPartyLoot(text)
		else
			player:sendTextMessage(MESSAGE_INFO_DESCR, text)
		end
	end
end

ec:register()
