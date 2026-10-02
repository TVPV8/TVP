// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "bed.h"
#include "game.h"
#include "iologindata.h"
#include "scheduler.h"

#include <fmt/format.h>

extern Game g_game;

Attr_ReadValue BedItem::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	switch (attr) {
		case ATTR_SLEEPERGUID: {
			uint32_t guid;
			if (!propStream.read<uint32_t>(guid)) {
				return ATTR_READ_ERROR;
			}

			if (guid != 0) {
				std::string name = IOLoginData::getNameByGuid(guid);
				if (!name.empty()) {
					setSpecialDescription(name + " is sleeping there.");
					g_game.setBedSleeper(this, guid);
					sleeperGUID = guid;
				}
			}
			return ATTR_READ_CONTINUE;
		}

		case ATTR_SLEEPSTART: {
			uint32_t sleep_start;
			if (!propStream.read<uint32_t>(sleep_start)) {
				return ATTR_READ_ERROR;
			}

			sleepStart = static_cast<uint64_t>(sleep_start);
			return ATTR_READ_CONTINUE;
		}

		default:
			break;
	}
	return Item::readAttr(attr, propStream);
}

void BedItem::serializeAttr(PropWriteStream& propWriteStream) const
{
	Item::serializeAttr(propWriteStream);

	if (sleeperGUID != 0) {
		propWriteStream.write<uint8_t>(ATTR_SLEEPERGUID);
		propWriteStream.write<uint32_t>(sleeperGUID);
	}

	if (sleepStart != 0) {
		propWriteStream.write<uint8_t>(ATTR_SLEEPSTART);
		// FIXME: should be stored as 64-bit, but we need to retain backwards compatibility
		propWriteStream.write<uint32_t>(static_cast<uint32_t>(sleepStart));
	}
}

BedItem* BedItem::getNextBedItem() const
{
	Direction dir = Item::items[id].bedPartnerDir;
	Position targetPos = getNextPosition(dir, getPosition());

	Tile* tile = g_game.map.getTile(targetPos);
	if (!tile) {
		return nullptr;
	}
	return tile->getBedItem();
}

bool BedItem::canUse(Player* player)
{
	if (!player || !house || !player->isPremium() || player->getZone() != ZONE_PROTECTION) {
		return false;
	}

	const ItemType& iType = Item::items[getID()];
	if (!(iType.bedPartnerDir == DIRECTION_SOUTH || iType.bedPartnerDir == DIRECTION_EAST)) {
		return false;
	}

	if (player->hasCondition(CONDITION_INFIGHT)) {
		player->sendCancelMessage(RETURNVALUE_CANNOTUSETHISOBJECT);
		return false;
	}

	if (sleeperGUID == 0) {
		return true;
	}

	Player sleeper(nullptr);
	if (!IOLoginData::loadPlayerByGUID(&sleeper, sleeperGUID)) {
		return false;
	}

	if (house->getHouseAccessLevel(&sleeper) > house->getHouseAccessLevel(player)) {
		return false;
	}

	sleeper.position = house->getEntryPosition();
	IOLoginData::savePlayer(&sleeper);
	return true;
}

bool BedItem::trySleep(Player* player)
{
	if (!house || player->isRemoved()) {
		return false;
	}

	if (player->hasCondition(CONDITION_INFIGHT)) {
		player->sendCancelMessage(RETURNVALUE_CANNOTUSETHISOBJECT);
		return false;
	}

	if (sleeperGUID != 0) {
		if (Item::items[id].transformToFree != 0 && house->getOwner() == player->getGUID()) {
			wakeUp(nullptr);
		}

		g_game.addMagicEffect(player->getPosition(), CONST_ME_POFF);
		return false;
	}
	return true;
}

bool BedItem::sleep(Player* player)
{
	if (!house) {
		return false;
	}

	if (sleeperGUID != 0) {
		return false;
	}

	internalSetSleeper(player);

	// update the bedSleepersMap
	g_game.setBedSleeper(this, player->getGUID());

	// make the player walk onto the bed
	g_game.map.moveCreature(*player, *getTile());

	// display poff effect
	g_game.addMagicEffect(player->getPosition(), CONST_ME_POFF);

	// kick player after he sees himself walk onto the bed and it change id
	uint32_t playerId = player->getID();
	g_scheduler.addEvent(
	    createSchedulerTask(SCHEDULER_MINTICKS, std::bind(&Game::kickPlayer, &g_game, playerId, false)));

	// change self and partner's appearance
	updateAppearance(player);

	if (BedItem* nextBedItem = getNextBedItem()) {
		nextBedItem->updateAppearance(player);
	}

	return true;
}

void BedItem::wakeUp(Player* player)
{
	if (!house) {
		return;
	}

	if (sleeperGUID != 0) {
		if (player) {
			regeneratePlayer(player);
			g_game.addCreatureHealth(player);
		}
	}

	// update the bedSleepersMap
	g_game.removeBedSleeper(sleeperGUID);

	BedItem* nextBedItem = getNextBedItem();

	// unset sleep info
	internalRemoveSleeper();

	if (nextBedItem) {
		nextBedItem->internalRemoveSleeper();
	}

	// change self and partner's appearance
	updateAppearance(nullptr);

	if (nextBedItem) {
		nextBedItem->updateAppearance(nullptr);
	}
}

void BedItem::regeneratePlayer(Player* player) const
{
	const uint32_t sleptTime = time(nullptr) - sleepStart;

	if (sleptTime < 60) {
		return;
	}

	int32_t regenAmount = sleptTime / 60;
	Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
	if (condition) {
		if (condition->getTicks() >= 1000) {
			if (condition->getTicks() * 5 < sleptTime * 1000) {
				regenAmount = (condition->getTicks() * 5) / (60 * 1000);
			}

			const int32_t newRegenTicks = condition->getTicks() - (1000 * (sleptTime / 5));
			if (newRegenTicks <= 0) {
				player->removeCondition(condition);
			} else {
				condition->setTicks(newRegenTicks);
			}
		}

		if (regenAmount > 0) {
			player->changeHealth(regenAmount, false);
			player->changeMana(regenAmount);
		}
	}

	if (sleptTime > 899) {
		player->changeSoul(sleptTime / 900);
	}
}

void BedItem::updateAppearance(const Player* player)
{
	const ItemType& it = Item::items[id];
	if (it.type == ITEM_TYPE_BED) {
		if (player && it.transformToOnUse[player->getSex()] != 0) {
			const ItemType& newType = Item::items[it.transformToOnUse[player->getSex()]];
			if (newType.type == ITEM_TYPE_BED) {
				g_game.transformItem(this, it.transformToOnUse[player->getSex()]);
			}
		} else if (it.transformToFree != 0) {
			const ItemType& newType = Item::items[it.transformToFree];
			if (newType.type == ITEM_TYPE_BED) {
				g_game.transformItem(this, it.transformToFree);
			}
		}
	}
}

void BedItem::internalSetSleeper(const Player* player)
{
	std::string desc_str = player->getName() + " is sleeping there.";

	sleeperGUID = player->getGUID();
	sleepStart = time(nullptr);
	setSpecialDescription(desc_str);
}

void BedItem::internalRemoveSleeper()
{
	sleeperGUID = 0;
	sleepStart = 0;

	if (hasAttribute(ITEM_ATTRIBUTE_DESCRIPTION)) {
		removeAttribute(ITEM_ATTRIBUTE_DESCRIPTION);
	}
}
