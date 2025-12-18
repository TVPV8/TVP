function onUseQuest(player, item, chest)
	local storageValue = chest.storageValue

	if player:getStorageValue(storageValue) ~= -1 then
		player:sendTextMessage(MESSAGE_INFO_DESCR, "The " .. item:getName() .. " is empty.")
		return true
	end

	local mainItemType = ItemType(chest.item.id)
	if not mainItemType then
		return false
	end
	
	local rewardName = mainItemType:getName()
	if mainItemType:isStackable() and chest.item.count > 1 then
		rewardName = chest.item.count .. " " .. mainItemType:getPluralName()
	end
	
	if mainItemType:getArticle():len() > 0 and math.max(1, chest.item.count or 0) <= 1 then
		rewardName = mainItemType:getArticle() .. " " .. rewardName
	end
	
	local rewardWeight = mainItemType:getWeight(chest.item.count)

	if chest.content then
		for _, reward in ipairs(chest.content) do
			local itemType = ItemType(reward.id)
			rewardWeight = rewardWeight + itemType:getWeight(reward.count)
		end
	end
	
	local term = "it is"
	if mainItemType:isStackable() and chest.item.count > 1 then
		term = "they are"
	end
	
	local noCapacityMessage = string.format("You have found %s. Weighing %d.%02d oz %s too heavy.", rewardName, rewardWeight / 100, rewardWeight % 100, term)
	
	if rewardWeight > player:getFreeCapacity() and not getPlayerFlagValue(player, PlayerFlag_HasInfiniteCapacity) then
		player:sendTextMessage(MESSAGE_INFO_DESCR, noCapacityMessage)
		return true
	end
	
	local reward = Game.createItem(mainItemType:getId(), chest.item.count or chest.item.subtype or chest.item.charges)
	if not reward then
		return false
	end

	if chest.item.text then
		reward:setAttribute(ITEM_ATTRIBUTE_TEXT, chest.item.text)
	end
	
	if chest.item.keynumber then
		reward:setAttribute(ITEM_ATTRIBUTE_KEYNUMBER, chest.item.keynumber)
	end
	
	if chest.content and mainItemType:isContainer() then
		for _, nextReward in ipairs(chest.content) do
			local nextItem = reward:addItem(nextReward.id, nextReward.count or nextReward.subtype or nextReward.charges)
			
			if nextReward.text then
				nextItem:setAttribute(ITEM_ATTRIBUTE_TEXT, nextReward.text)
			end
			
			if nextReward.keynumber then
				nextItem:setAttribute(ITEM_ATTRIBUTE_KEYNUMBER, nextReward.keynumber)
			end
		end
	end
	
	if player:getFreeCapacity() >= reward:getWeight() then
		if player:addItemEx(reward) == RETURNVALUE_NOERROR then
			player:sendTextMessage(MESSAGE_INFO_DESCR, "You have found " .. rewardName .. ".")
			
			if not getPlayerFlagValue(player, PlayerFlag_HasInfiniteCapacity) then
				player:setStorageValue(storageValue, 1)
			end
		else
			player:sendTextMessage(MESSAGE_INFO_DESCR, "You have found " .. rewardName .. ", but you have no room to take it.")
			reward:remove()
		end
	else
		player:sendTextMessage(MESSAGE_INFO_DESCR, noCapacityMessage)
		reward:remove()
	end
	return true
end

function destroyItem(player, target, toPosition)
	if type(target) ~= "userdata" or not target:isItem() then
		return false
	end

	if target:hasAttribute(ITEM_ATTRIBUTE_UNIQUEID) or target:hasAttribute(ITEM_ATTRIBUTE_ACTIONID) then
		return false
	end

	if toPosition.x == CONTAINER_POSITION then
		player:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
		return true
	end

	local destroyId = ItemType(target.itemid):getDestroyId()
	if destroyId == 0 then
		return false
	end

	if math.random(7) == 1 then
		local item = Game.createItem(destroyId, 1, toPosition)
		if item then
			item:decay()
		end

		-- Move items outside the container
		if target:isContainer() then
			for i = target:getSize() - 1, 0, -1 do
				local containerItem = target:getItem(i)
				if containerItem then
					containerItem:moveTo(toPosition)
				end
			end
		end

		target:remove(1)
	end

	toPosition:sendMagicEffect(CONST_ME_POFF)
	return true
end

function onUseMachete(player, item, fromPosition, target, toPosition)
	local targetId = target.itemid
	if not targetId then
		return true
	end

	if targetId == 1499 then
		toPosition:sendMagicEffect(CONST_ME_POFF)
		target:remove()
		return true
	end

	local grass = jungleGrass[targetId]
	if grass then
		target:transform(grass)
		target:decay()
		return true
	end

	return destroyItem(player, target, toPosition)
end

function onUsePick(player, item, fromPosition, target, toPosition)
	local tile = Tile(toPosition)
	if not tile then
		return false
	end
	
	if tile:getItemByGroup(ITEM_GROUP_SPLASH) then
		target = tile:getGround()
	end

	if table.contains(pickGrounds, target.itemid) and target.actionid == actionIds.pickHole then
		target:transform(392) -- decays automatically into normal dirt tile
		target:decay()

		toPosition.z = toPosition.z + 1
		tile:relocateTo(toPosition)
		return true
	end

	return false
end

function onUseKnife(player, item, fromPosition, target, toPosition)
	if not target:isItem() then
		return false
	end
	
	if target:getId() == 2683 then 
		target:transform(2096, 1)
		target:decay()
		return true
	end
	
	return false
end

function onUseRope(player, item, fromPosition, target, toPosition)
	local tile = Tile(toPosition)
	if not tile then
		return false
	end
	
	local itemType = ItemType(target.itemid)
	if itemType and itemType:getGroup() == ITEM_GROUP_SPLASH then
		target = tile:getGround()
	end

	if table.contains(ropeSpots, target.itemid) then
		tile = Tile(toPosition:moveUpstairs())
		if not tile then
			return false
		end

		if tile:hasFlag(TILESTATE_PROTECTIONZONE) and player:isPzLocked() then
			player:sendCancelMessage(RETURNVALUE_PLAYERISPZLOCKED)
			return true
		end

		player:teleportTo(toPosition, false)
		return true
	end

	if table.contains(holeId, target.itemid) then
		toPosition.z = toPosition.z + 1
		tile = Tile(toPosition)
		if not tile then
			return false
		end

		local thing = tile:getBottomCreature() or tile:getTopDownItem()
		if not thing then
			return true
		end

		if thing:isCreature() then
			return thing:teleportTo(toPosition:moveUpstairs(), false)
		elseif thing:isItem() and thing:getType():isMovable() then
			return thing:moveTo(toPosition:moveUpstairs())
		end

		return true
	end

	return false
end

function onUseShovel(player, item, fromPosition, target, toPosition)
	local tile = Tile(toPosition)
	if not tile then
		return false
	end
	
	local itemType = ItemType(target.itemid)
	if itemType:getGroup() == ITEM_GROUP_SPLASH then
		target = tile:getGround()
	end

	if table.contains(holes, target.itemid) then
		target:transform(target.itemid + 1)
		target:decay()

		toPosition.z = toPosition.z + 1
		tile:relocateTo(toPosition)
	elseif table.contains(sandIds, target.itemid) then
		local randomValue = randomNumber(1, 100)
		if target.actionid == actionIds.sandHole and randomValue <= 20 then
			target:transform(489)
			target:decay()
		else
			checkScarabTile(toPosition)
		end
		toPosition:sendMagicEffect(CONST_ME_POFF)
	else
		return false
	end

	return true
end

function onUseScythe(player, item, fromPosition, target, toPosition)
	if target.itemid == 2739 then -- wheat
		target:transform(2737)
		target:decay()
		Game.createItem(2694, 1, toPosition) -- bunch of wheat
		return true
	elseif target.itemid == 2738 then -- growing wheat
		player:sendCancelMessage("It's not mature yet.")
		return true
	end
	return destroyItem(player, target, toPosition)
end

local logFormat = "[%s] %s %s"

function logCommand(player, words, param)
	local file = io.open("data/logs/" .. player:getName() .. " commands.log", "a")
	if not file then
		return
	end

	io.output(file)
	io.write(logFormat:format(os.date("%d/%m/%Y %H:%M"), words, param):trim() .. "\n")
	io.close(file)
end

function Player:addPartyCondition(combat, variant, condition, baseMana)
	local party = self:getParty()
	if not party then
		self:sendCancelMessage(RETURNVALUE_NOPARTYMEMBERSINRANGE)
		self:getPosition():sendMagicEffect(CONST_ME_POFF)
		return false
	end

	local positions = combat:getPositions(self, variant)
	local members = party:getMembers()
	members[#members + 1] = party:getLeader()

	local affectedMembers = {}
	for _, member in ipairs(members) do
		local memberPosition = member:getPosition()
		for _, position in ipairs(positions) do
			if memberPosition == position then
				affectedMembers[#affectedMembers + 1] = member
			end
		end
	end

	if #affectedMembers <= 1 then
		self:sendCancelMessage(RETURNVALUE_NOPARTYMEMBERSINRANGE)
		self:getPosition():sendMagicEffect(CONST_ME_POFF)
		return false
	end

	local mana = math.ceil(#affectedMembers * math.pow(0.9, #affectedMembers - 1) * baseMana)
	if self:getMana() < mana then
		self:sendCancelMessage(RETURNVALUE_NOTENOUGHMANA)
		self:getPosition():sendMagicEffect(CONST_ME_POFF)
		return false
	end

	self:addMana(-mana)
	self:addManaSpent(mana)

	for _, member in ipairs(affectedMembers) do
		member:addCondition(condition)
	end

	for _, position in ipairs(positions) do
		position:sendMagicEffect(CONST_ME_MAGIC_BLUE)
	end
	return true
end

function Player:conjureItem(conjureMana, reagentId, conjureId, conjureCount, effect)
	local conjureFromHandsOnly = true
	
	if not conjureCount and conjureId ~= 0 then
		local itemType = ItemType(conjureId)
		if itemType:getId() == 0 then
			return false
		end

		local charges = itemType:getCharges()
		if charges ~= 0 then
			conjureCount = charges
		end
	end

	if not conjureFromHandsOnly then
		local reagent = self:getItemById(reagentId, true)
		if reagentId ~= 0 and not reagent then
			self:sendCancelMessage(RETURNVALUE_YOUNEEDAMAGICITEMTOCASTSPELL)
			self:getPosition():sendMagicEffect(CONST_ME_POFF)
			return false
		end

		local item = reagent
		if reagentId == 0 then
			item = self:addItem(conjureId, conjureCount)
			if not item then
				self:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
				self:getPosition():sendMagicEffect(CONST_ME_POFF)
				return false
			end
		else
			reagent:transform(conjureId, conjureCount)
		end

		if item:hasAttribute(ITEM_ATTRIBUTE_DURATION) then
			item:decay()
		end

		self:getPosition():sendMagicEffect(effect or CONST_ME_MAGIC_RED)
	else
		local leftItem = self:getSlotItem(CONST_SLOT_LEFT)
		local rightItem = self:getSlotItem(CONST_SLOT_RIGHT)
		local totalConjures = 0
		
		if reagentId ~= 0 then
			if leftItem and leftItem:getId() == reagentId then
				leftItem:remove()
				
				local item = self:addItem(conjureId, 1, true, conjureCount, CONST_SLOT_LEFT)
				if not item then
					self:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
					self:getPosition():sendMagicEffect(CONST_ME_POFF)
					return false
				end

				if item:hasAttribute(ITEM_ATTRIBUTE_DURATION) then
					item:decay()
				end

				self:getPosition():sendMagicEffect(effect or CONST_ME_MAGIC_RED)
				totalConjures = 1
			end
			
			if rightItem and rightItem:getId() == reagentId then
				local infiniteMana = self:getGroup():hasFlag(PlayerFlag_HasInfiniteMana)
				local notEnoughMana = self:getMana() < conjureMana * 2 and not infiniteMana
				
				if totalConjures == 1 and notEnoughMana then
					-- not enough mana on second hand
					return true
				elseif totalConjures == 1 then
					-- take mana for second hand
					self:addMana(-conjureMana)
					self:addManaSpent(conjureMana)
				end

				rightItem:remove()
				
				local item = self:addItem(conjureId, 1, true, conjureCount, CONST_SLOT_RIGHT)
				if not item then
					self:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
					self:getPosition():sendMagicEffect(CONST_ME_POFF)
					return false
				end

				if item:hasAttribute(ITEM_ATTRIBUTE_DURATION) then
					item:decay()
				end

				self:getPosition():sendMagicEffect(effect or CONST_ME_MAGIC_RED)
				totalConjures = totalConjures + 1
			end
			
			if totalConjures == 0 then
				self:sendCancelMessage(RETURNVALUE_YOUNEEDAMAGICITEMTOCASTSPELL)
				self:getPosition():sendMagicEffect(CONST_ME_POFF)
				return false
			end
		else
			local item = self:addItem(conjureId, conjureCount)
			if not item then
				self:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
				self:getPosition():sendMagicEffect(CONST_ME_POFF)
				return false
			end

			if item:hasAttribute(ITEM_ATTRIBUTE_DURATION) then
				item:decay()
			end

			self:getPosition():sendMagicEffect(effect or CONST_ME_MAGIC_RED)
		end
	end
	
	return true
end

function Player:computeSkillDamage(damage, variation, skill, limitMinimum, limitMaximum)
	local min = damage - variation
	local max = damage + variation
	local formula = (3 * self:getMagicLevel()) + 2 * self:getLevel()
		
	if limitMinimum and formula <= 99 or limitMaximum and formula >= 101 then
		formula = 100
	end

	min = formula * min / 100
	max = formula * max / 100

	-- skills
	min = min * self:getLevel() / 25
	max = max * self:getLevel() / 25
	return -min, -max
end

function Player:computeDamage(damage, variation, limitMinimum, limitMaximum)
	local minimumDamage = damage - variation
	local maximumDamage = damage + variation
	local min = minimumDamage
	local max = maximumDamage
	local formula = (3 * self:getMagicLevel()) + 2 * self:getLevel()

	if limitMinimum and formula <= 99 or limitMaximum and formula >= 101 then
		formula = 100
	end

	min = formula * min / 100
	max = formula * max / 100

	return -min, -max
end

function Player:computeHealing(damage, variation, limitMinimum, limitMaximum)
	local minimumDamage = damage - variation
	local maximumDamage = damage + variation
	local min = minimumDamage
	local max = maximumDamage
	local formula = (3 * self:getMagicLevel()) + 2 * self:getLevel()

	if limitMinimum and formula <= 99 or limitMaximum and formula >= 101 then
		formula = 100
	end

	min = formula * min / 100
	max = formula * max / 100
	return min, max
end

function Creature:addAttributeCondition(parameters)
	local condition = Condition(CONDITION_ATTRIBUTES)
	for _, parameter in ipairs(parameters) do
		if parameter.key and parameter.value then
			condition:setParameter(parameter.key, parameter.value)
		end
	end

	self:addCondition(condition)
end
