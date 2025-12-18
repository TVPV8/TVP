local creatureevent = CreatureEvent("DropLoot")

function creatureevent.onDeath(player, corpse, killer, mostDamageKiller, lastHitUnjustified, mostDamageUnjustified)
	if player:hasFlag(PlayerFlag_NotGenerateLoot) then
		return true
	end

	local amulet = player:getSlotItem(CONST_SLOT_NECKLACE)
	local isRed = player:getSkull() == SKULL_RED
	
	if amulet and amulet.itemid == ITEM_AMULETOFLOSS and not isRed then
		amulet:remove(1)
	else
		for i = CONST_SLOT_HEAD, CONST_SLOT_AMMO do
			local item = player:getSlotItem(i)
			local lossPercent = player:getLossPercent()
			
			if configManager.getBoolean(configKeys.CLASSIC_PLAYER_LOOTDROP) then
				if item then
					if isRed or math.random(0, 9) == 0 or item:isContainer() then
						if not item:moveTo(corpse) then
							item:remove()
						end
					end
				end
			else
				if item then
					if isRed or math.random(item:isContainer() and 100 or 1000) <= lossPercent then
						if (isRed or lossPercent ~= 0) and not item:moveTo(corpse) then
							item:remove()
						end
					end
				end
			end
		end
	end

	return true
end

creatureevent:register()