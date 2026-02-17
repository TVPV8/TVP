// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "iologindata.h"
#include "configmanager.h"
#include "game.h"

#include <fmt/format.h>


extern ConfigManager g_config;
extern Game g_game;

static std::map<std::string, slots_t> stringToSlot = {
	{ "head", CONST_SLOT_HEAD },
	{ "armor", CONST_SLOT_ARMOR},
	{ "legs", CONST_SLOT_LEGS },
	{ "feet", CONST_SLOT_FEET },
	{ "right", CONST_SLOT_RIGHT },
	{ "left", CONST_SLOT_LEFT },
	{ "amulet", CONST_SLOT_NECKLACE },
	{ "backpack", CONST_SLOT_BACKPACK },
	{ "ammo", CONST_SLOT_AMMO },
	{ "ring", CONST_SLOT_RING },
};

static std::map<slots_t, std::string> slotToString = {
	{ CONST_SLOT_HEAD, "Head" },
	{ CONST_SLOT_ARMOR, "Armor" },
	{ CONST_SLOT_LEGS, "Legs" },
	{ CONST_SLOT_FEET, "Feet" },
	{ CONST_SLOT_RIGHT, "Right" },
	{ CONST_SLOT_LEFT, "Left" },
	{ CONST_SLOT_NECKLACE, "Amulet" },
	{ CONST_SLOT_BACKPACK, "Backpack" },
	{ CONST_SLOT_RING, "Ring" },
	{ CONST_SLOT_AMMO, "Ammo" },
};

Account IOLoginData::loadAccount(uint32_t accno)
{
	Account account;

	DBResult_ptr result = Database::getInstance().storeQuery(fmt::format("SELECT `id`, `password`, `type`, `premium_ends_at` FROM `accounts` WHERE `id` = {:d}", accno));
	if (!result) {
		return account;
	}

	account.id = result->getNumber<uint32_t>("id");
	account.accountType = static_cast<AccountType_t>(result->getNumber<int32_t>("type"));
	account.premiumEndsAt = result->getNumber<time_t>("premium_ends_at");
	return account;
}

bool IOLoginData::loginserverAuthentication(uint32_t accountNumber, const std::string& password, Account& account)
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery(fmt::format("SELECT `id`, `password`, `type`, `premium_ends_at` FROM `accounts` WHERE `id` = {:d}", accountNumber));
	if (!result) {
		return false;
	}

	if (transformToSHA1(password) != result->getString("password")) {
		return false;
	}

	account.id = result->getNumber<uint32_t>("id");
	account.accountType = static_cast<AccountType_t>(result->getNumber<int32_t>("type"));
	account.premiumEndsAt = result->getNumber<time_t>("premium_ends_at");

	result = db.storeQuery(fmt::format("SELECT `name` FROM `players` WHERE `account_id` = {:d} AND `deletion` = 0 ORDER BY `name` ASC", account.id));
	if (result) {
		do {
			account.characters.push_back(result->getString("name"));
		} while (result->next());
	}
	return true;
}

uint32_t IOLoginData::gameworldAuthentication(uint32_t accountNumber, const std::string& password, std::string& characterName)
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery(fmt::format("SELECT `id`, `password` FROM `accounts` WHERE `id` = {:d}", accountNumber));
	if (!result) {
		return 0;
	}

	if (transformToSHA1(password) != result->getString("password")) {
		return 0;
	}

	uint32_t accountId = result->getNumber<uint32_t>("id");

	result = db.storeQuery(fmt::format("SELECT `name` FROM `players` WHERE `name` = {:s} AND `account_id` = {:d} AND `deletion` = 0", db.escapeString(characterName), accountId));
	if (!result) {
		return 0;
	}

	characterName = result->getString("name");
	return accountId;
}

uint32_t IOLoginData::getAccountIdByPlayerName(const std::string& playerName)
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery(fmt::format("SELECT `account_id` FROM `players` WHERE `name` = {:s}", db.escapeString(playerName)));
	if (!result) {
		return 0;
	}
	return result->getNumber<uint32_t>("account_id");
}

uint32_t IOLoginData::getAccountIdByPlayerId(uint32_t playerId)
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery(fmt::format("SELECT `account_id` FROM `players` WHERE `id` = {:d}", playerId));
	if (!result) {
		return 0;
	}
	return result->getNumber<uint32_t>("account_id");
}

AccountType_t IOLoginData::getAccountType(uint32_t accountId)
{
	DBResult_ptr result = Database::getInstance().storeQuery(fmt::format("SELECT `type` FROM `accounts` WHERE `id` = {:d}", accountId));
	if (!result) {
		return ACCOUNT_TYPE_NORMAL;
	}
	return static_cast<AccountType_t>(result->getNumber<uint16_t>("type"));
}

void IOLoginData::setAccountType(uint32_t accountId, AccountType_t accountType)
{
	Database::getInstance().executeQuery(fmt::format("UPDATE `accounts` SET `type` = {:d} WHERE `id` = {:d}", static_cast<uint16_t>(accountType), accountId));
}

void IOLoginData::updateOnlineStatus(uint32_t guid, bool login)
{
	if (g_config.getBoolean(ConfigManager::ALLOW_CLONES)) {
		return;
	}

	if (login) {
		Database::getInstance().executeQuery(fmt::format("INSERT INTO `players_online` VALUES ({:d})", guid));
	} else {
		Database::getInstance().executeQuery(fmt::format("DELETE FROM `players_online` WHERE `player_id` = {:d}", guid));
	}
}

bool IOLoginData::preloadPlayer(Player* player, const std::string& name)
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery(fmt::format("SELECT `p`.`id`, `p`.`sex`, `p`.`vocation`, `p`.`town_id`, `p`.`account_id`, `p`.`group_id`, `a`.`type`, `a`.`premium_ends_at` FROM `players` as `p` JOIN `accounts` as `a` ON `a`.`id` = `p`.`account_id` WHERE `p`.`name` = {:s} AND `p`.`deletion` = 0", db.escapeString(name)));
	if (!result) {
		return false;
	}

	Group* group = g_game.groups.getGroup(result->getNumber<uint16_t>("group_id"));
	if (!group) {
		std::cout << "[Error - IOLoginData::preloadPlayer] " << player->name << " has Group ID " << result->getNumber<uint16_t>("group_id") << " which doesn't exist." << std::endl;
		return false;
	}

	// Due to OTServers AACs, we have to set these from the DB all the time
	player->setGUID(result->getNumber<uint32_t>("id"));
	player->setGroup(group);
	player->setSex(static_cast<PlayerSex_t>(result->getNumber<int32_t>("sex")));
	player->setVocation(result->getNumber<uint16_t>("vocation"));
	player->setTown(g_game.map.towns.getTown(result->getNumber<uint16_t>("town_id")));
	//----

	player->accountNumber = result->getNumber<uint32_t>("account_id");
	player->accountType = static_cast<AccountType_t>(result->getNumber<uint16_t>("type"));
	player->premiumEndsAt = result->getNumber<time_t>("premium_ends_at");
	return true;
}

bool IOLoginData::loadPlayerByGUID(Player* player, uint32_t id)
{
	player->guid = id;
	return loadPlayer(player, false);
}

bool IOLoginData::loadPlayerByName(Player* player, const std::string& name)
{
	Database& db = Database::getInstance();
	DBResult_ptr result = db.storeQuery(fmt::format("SELECT `id`, `name` FROM `players` WHERE `name` = {:s}", db.escapeString(name)));
	if (!result) {
		return false;
	}

	player->name = result->getString("name");
	player->guid = result->getNumber<uint32_t>("id");
	return loadPlayer(player, false);
}

bool IOLoginData::loadPlayer(Player* player, bool /*initializeScriptFile*/)
{
	Database& db = Database::getInstance();

	// Load main player data from players table
	DBResult_ptr result = db.storeQuery(fmt::format(
		"SELECT `id`, `name`, `group_id`, `account_id`, `level`, `vocation`, `health`, `healthmax`, "
		"`experience`, `lookbody`, `lookfeet`, `lookhead`, `looklegs`, `looktype`, `maglevel`, "
		"`mana`, `manamax`, `manaspent`, `soul`, `town_id`, `posx`, `posy`, `posz`, `sex`, "
		"`lastlogin`, `lastlogout`, `skull`, `skulltime`, `balance`, `stamina`, `blessings`, `cap`, "
		"`skill_fist`, `skill_fist_tries`, `skill_club`, `skill_club_tries`, `skill_sword`, `skill_sword_tries`, "
		"`skill_axe`, `skill_axe_tries`, `skill_dist`, `skill_dist_tries`, `skill_shielding`, `skill_shielding_tries`, "
		"`skill_fishing`, `skill_fishing_tries`, `conditions`, `unjusts` "
		"FROM `players` WHERE `id` = {:d}", player->getGUID()));

	if (!result) {
		return false;
	}

	// Basic player data
	player->name = result->getString("name");
	std::string unjustsString = result->getString("unjusts");
	
	Group* group = g_game.groups.getGroup(result->getNumber<uint16_t>("group_id"));
	if (!group) {
		std::cout << "[Error - IOLoginData::loadPlayer] " << player->name << " has Group ID " << result->getNumber<uint16_t>("group_id") << " which doesn't exist." << std::endl;
		return false;
	}
	player->setGroup(group);

	player->accountNumber = result->getNumber<uint32_t>("account_id");
	player->level = result->getNumber<uint32_t>("level");
	
	if (!player->setVocation(result->getNumber<uint16_t>("vocation"))) {
		std::cout << "[Error - IOLoginData::loadPlayer] " << player->name << " has invalid vocation ID: " << result->getNumber<uint16_t>("vocation") << std::endl;
		return false;
	}

	player->health = result->getNumber<int32_t>("health");
	player->healthMax = result->getNumber<int32_t>("healthmax");
	player->experience = result->getNumber<uint64_t>("experience");

	// Outfit
	player->defaultOutfit.lookBody = result->getNumber<uint32_t>("lookbody");
	player->defaultOutfit.lookFeet = result->getNumber<uint32_t>("lookfeet");
	player->defaultOutfit.lookHead = result->getNumber<uint32_t>("lookhead");
	player->defaultOutfit.lookLegs = result->getNumber<uint32_t>("looklegs");
	player->defaultOutfit.lookType = result->getNumber<uint32_t>("looktype");
	player->currentOutfit = player->defaultOutfit;

	player->magLevel = result->getNumber<uint32_t>("maglevel");
	player->mana = result->getNumber<uint32_t>("mana");
	player->manaMax = result->getNumber<uint32_t>("manamax");
	player->manaSpent = result->getNumber<uint64_t>("manaspent");
	player->soul = static_cast<uint8_t>(result->getNumber<uint16_t>("soul"));

	Town* town = g_game.map.towns.getTown(result->getNumber<uint16_t>("town_id"));
	if (!town) {
		std::cout << "[Error - IOLoginData::loadPlayer] " << player->name << " has invalid town ID: " << result->getNumber<uint16_t>("town_id") << std::endl;
		return false;
	}
	player->setTown(town);

	// Position
	uint16_t posx = result->getNumber<uint16_t>("posx");
	uint16_t posy = result->getNumber<uint16_t>("posy");
	uint8_t posz = static_cast<uint8_t>(result->getNumber<uint16_t>("posz"));
	if (posx != 0 && posy != 0 && posz != 0) {
		player->position = Position(posx, posy, posz);
		player->loginPosition = player->position;
	} else {
		player->position = player->getTemplePosition();
		player->loginPosition = player->position;
	}

	player->setSex(static_cast<PlayerSex_t>(result->getNumber<int32_t>("sex")));
	player->lastLoginSaved = result->getNumber<time_t>("lastlogin");
	player->lastLogout = result->getNumber<time_t>("lastlogout");
	player->setSkull(static_cast<Skulls_t>(result->getNumber<int32_t>("skull")));
	player->playerKillerEnd = result->getNumber<time_t>("skulltime");
	player->bankBalance = result->getNumber<uint64_t>("balance");
	player->staminaMinutes = result->getNumber<uint16_t>("stamina");
	player->blessings = static_cast<uint8_t>(result->getNumber<uint16_t>("blessings"));
	player->capacity = result->getNumber<uint32_t>("cap");

	// Skills
	player->skills[SKILL_FIST].level = result->getNumber<uint16_t>("skill_fist");
	player->skills[SKILL_FIST].tries = result->getNumber<uint64_t>("skill_fist_tries");
	player->skills[SKILL_CLUB].level = result->getNumber<uint16_t>("skill_club");
	player->skills[SKILL_CLUB].tries = result->getNumber<uint64_t>("skill_club_tries");
	player->skills[SKILL_SWORD].level = result->getNumber<uint16_t>("skill_sword");
	player->skills[SKILL_SWORD].tries = result->getNumber<uint64_t>("skill_sword_tries");
	player->skills[SKILL_AXE].level = result->getNumber<uint16_t>("skill_axe");
	player->skills[SKILL_AXE].tries = result->getNumber<uint64_t>("skill_axe_tries");
	player->skills[SKILL_DISTANCE].level = result->getNumber<uint16_t>("skill_dist");
	player->skills[SKILL_DISTANCE].tries = result->getNumber<uint64_t>("skill_dist_tries");
	player->skills[SKILL_SHIELD].level = result->getNumber<uint16_t>("skill_shielding");
	player->skills[SKILL_SHIELD].tries = result->getNumber<uint64_t>("skill_shielding_tries");
	player->skills[SKILL_FISHING].level = result->getNumber<uint16_t>("skill_fishing");
	player->skills[SKILL_FISHING].tries = result->getNumber<uint64_t>("skill_fishing_tries");

	// Load conditions
	unsigned long condSize;
	const char* condData = result->getStream("conditions", condSize);
	if (condData && condSize > 0) {
		PropStream propStream;
		propStream.init(condData, condSize);

		uint32_t conditionCount;
		if (propStream.read<uint32_t>(conditionCount)) {
			for (uint32_t i = 0; i < conditionCount; i++) {
				uint8_t condType;
				if (!propStream.read<uint8_t>(condType)) {
					break;
				}

				Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, static_cast<ConditionType_t>(condType), 0);
				if (!condition) {
					break;
				}

				if (!condition->unserialize(propStream)) {
					delete condition;
					break;
				}

				// Never load in-fight condition
				if (static_cast<ConditionType_t>(condType) == CONDITION_INFIGHT) {
					delete condition;
				} else {
					player->storedConditionList.push_front(condition);
				}
			}
		}
	}

	// Load storage values
	if ((result = db.storeQuery(fmt::format("SELECT `key`, `value` FROM `player_storage` WHERE `player_id` = {:d}", player->getGUID())))) {
		do {
			player->storageMap[result->getNumber<uint32_t>("key")] = result->getNumber<int32_t>("value");
		} while (result->next());
	}

	// Load string storage values
	if ((result = db.storeQuery(fmt::format("SELECT `key`, `value` FROM `player_string_storage` WHERE `player_id` = {:d}", player->getGUID())))) {
		do {
			player->stringStorageMap[result->getString("key")] = result->getString("value");
		} while (result->next());
	}

	// Load spells
	if ((result = db.storeQuery(fmt::format("SELECT `name` FROM `player_spells` WHERE `player_id` = {:d}", player->getGUID())))) {
		do {
			player->learnedInstantSpellList.push_front(result->getString("name"));
		} while (result->next());
	}

	// Load VIP list
	if ((result = db.storeQuery(fmt::format("SELECT `vip_id` FROM `player_viplist` WHERE `player_id` = {:d}", player->getGUID())))) {
		do {
			uint32_t vipId = result->getNumber<uint32_t>("vip_id");
			player->VIPList.insert(vipId);
		} while (result->next());
	}

	// Validate VIP entries and cache names
	std::vector<uint32_t> invalidVIPEntries;
	for (const uint32_t& vip : player->VIPList) {
		if (DBResult_ptr vipResult = db.storeQuery(fmt::format("SELECT `name` FROM `players` WHERE `id` = {:d}", vip))) {
			g_game.storePlayerName(vip, vipResult->getString("name"));
		} else {
			invalidVIPEntries.push_back(vip);
		}
	}
	for (const uint32_t& vip : invalidVIPEntries) {
		player->VIPList.erase(vip);
	}

	// Load murders
	if (!unjustsString.empty()) {
		std::stringstream ss(unjustsString);
		std::string timestampStr;
		while (std::getline(ss, timestampStr, ',')) {
			try {
				player->murderTimeStamps.push_back(std::stoll(timestampStr));
			} catch (const std::exception&) {
				// Ignore invalid timestamps
			}
		}
	}

	// Load inventory items
	if ((result = db.storeQuery(fmt::format("SELECT `pid`, `itemtype`, `count`, `attributes` FROM `player_items` WHERE `player_id` = {:d} ORDER BY `sid` ASC", player->getGUID())))) {
		do {
			int32_t slot = result->getNumber<int32_t>("pid");
			uint16_t itemType = result->getNumber<uint16_t>("itemtype");
			uint16_t count = result->getNumber<uint16_t>("count");
			
			Item* item = Item::CreateItem(itemType, count);
			if (!item) {
				continue;
			}

			// Load item attributes from BLOB if present
			unsigned long attrSize;
			const char* attrData = result->getStream("attributes", attrSize);
			if (attrData && attrSize > 0) {
				PropStream propStream;
				propStream.init(attrData, attrSize);
				item->unserializeTVPFormat(propStream);
			}

			if (slot >= CONST_SLOT_FIRST && slot <= CONST_SLOT_LAST) {
				player->internalAddThing(slot, item);
				item->startDecaying();
			}
		} while (result->next());
	}

	// Load depot items
	if ((result = db.storeQuery(fmt::format("SELECT `depot_id`, `items` FROM `player_depotitems` WHERE `player_id` = {:d}", player->getGUID())))) {
		do {
			uint32_t depotId = result->getNumber<uint32_t>("depot_id");
			DepotLocker* depot = player->getDepotLocker(depotId, true);

			unsigned long itemsSize;
			const char* itemsData = result->getStream("items", itemsSize);
			if (itemsData && itemsSize > 0) {
				PropStream propStream;
				propStream.init(itemsData, itemsSize);
				
				// Read number of items
				uint32_t itemCount;
				if (propStream.read<uint32_t>(itemCount)) {
					for (uint32_t i = 0; i < itemCount; i++) {
						uint16_t itemId;
						if (!propStream.read<uint16_t>(itemId)) {
							break;
						}

						Item* item = Item::CreateItem(itemId);
						if (!item) {
							break;
						}

						if (!item->unserializeTVPFormat(propStream)) {
							delete item;
							break;
						}

						depot->internalAddThing(item);
					}
				}
			}
		} while (result->next());
	}



	// Load guild membership
	if ((result = db.storeQuery(fmt::format("SELECT `guild_id`, `rank_id`, `nick` FROM `guild_membership` WHERE `player_id` = {:d}", player->getGUID())))) {
		uint32_t guildId = result->getNumber<uint32_t>("guild_id");
		uint32_t playerRankId = result->getNumber<uint32_t>("rank_id");
		player->guildNick = result->getString("nick");

		Guild* guild = g_game.getGuild(guildId);
		if (!guild) {
			guild = IOGuild::loadGuild(guildId);
			if (guild) {
				g_game.addGuild(guild);
			} else {
				std::cout << "[Warning - IOLoginData::loadPlayer] " << player->name << " has Guild ID " << guildId << " which doesn't exist" << std::endl;
			}
		}

		if (guild) {
			player->guild = guild;
			GuildRank_ptr rank = guild->getRankById(playerRankId);
			if (!rank) {
				if ((result = db.storeQuery(fmt::format("SELECT `id`, `name`, `level` FROM `guild_ranks` WHERE `id` = {:d}", playerRankId)))) {
					guild->addRank(result->getNumber<uint32_t>("id"), result->getString("name"), result->getNumber<uint16_t>("level"));
				}

				rank = guild->getRankById(playerRankId);
				if (!rank) {
					player->guild = nullptr;
				}
			}

			player->guildRank = rank;

			IOGuild::getWarList(guildId, player->guildWarVector);

			if ((result = db.storeQuery(fmt::format("SELECT COUNT(*) AS `members` FROM `guild_membership` WHERE `guild_id` = {:d}", guildId)))) {
				guild->setMemberCount(result->getNumber<uint32_t>("members"));
			}
		}
	}

	// Calculate skills percentages to the next level
	uint64_t experience = player->experience;
	uint64_t currExpCount = Player::getExpForLevel(player->level);
	uint64_t nextExpCount = Player::getExpForLevel(player->level + 1);
	if (experience < currExpCount || experience > nextExpCount) {
		experience = currExpCount;
	}

	if (currExpCount < nextExpCount) {
		player->levelPercent = Player::getPercentLevel(player->experience - currExpCount, nextExpCount - currExpCount);
	} else {
		player->levelPercent = 0;
	}

	uint64_t nextManaCount = player->vocation->getReqMana(player->magLevel + 1);
	uint64_t manaSpent = player->manaSpent;
	if (manaSpent > nextManaCount) {
		manaSpent = 0;
	}

	player->magLevelPercent = Player::getPercentLevel(player->manaSpent, nextManaCount);

	for (int32_t i = SKILL_FIRST; i <= SKILL_LAST; i++) {
		uint16_t skillLevel = player->skills[i].level;
		uint64_t skillTries = player->skills[i].tries;
		uint64_t nextSkillTries = player->vocation->getReqSkillTries(i, skillLevel + 1);
		if (skillTries > nextSkillTries) {
			skillTries = 0;
		}

		player->skills[i].percent = Player::getPercentLevel(skillTries, nextSkillTries);
	}

	player->updateBaseSpeed();
	player->updateInventoryWeight();
	return true;
}


bool IOLoginData::savePlayer(Player* player)
{
	Database& db = Database::getInstance();
	std::ostringstream query;

	// Start transaction - all save operations are atomic
	DBTransaction transaction;
	if (!transaction.begin()) {
		return false;
	}

	// Serialize conditions
	PropWriteStream condStream;
	uint32_t conditionCount = 0;

	// Count persistable conditions
	for (Condition* condition : player->conditions) {
		if (condition->isPersistent()) {
			conditionCount++;
		}
	}
	for (Condition* condition : player->storedConditionList) {
		conditionCount++;
	}

	if (conditionCount > 0) {
		condStream.write<uint32_t>(conditionCount);

		for (Condition* condition : player->conditions) {
			if (condition->isPersistent()) {
				condStream.write<uint8_t>(static_cast<uint8_t>(condition->getType()));
				condition->serialize(condStream);
			}
		}
		for (Condition* condition : player->storedConditionList) {
			condStream.write<uint8_t>(static_cast<uint8_t>(condition->getType()));
			condition->serialize(condStream);
		}
	}

	size_t condSize;
	const char* condData = condStream.getStream(condSize);

	// Update main player data
	query << "UPDATE `players` SET ";
	query << "`level` = " << player->level << ',';
	query << "`group_id` = " << player->group->id << ',';
	query << "`vocation` = " << player->getVocationId() << ',';
	query << "`health` = " << player->health << ',';
	query << "`healthmax` = " << player->healthMax << ',';
	query << "`experience` = " << player->experience << ',';
	query << "`lookbody` = " << static_cast<uint32_t>(player->defaultOutfit.lookBody) << ',';
	query << "`lookfeet` = " << static_cast<uint32_t>(player->defaultOutfit.lookFeet) << ',';
	query << "`lookhead` = " << static_cast<uint32_t>(player->defaultOutfit.lookHead) << ',';
	query << "`looklegs` = " << static_cast<uint32_t>(player->defaultOutfit.lookLegs) << ',';
	query << "`looktype` = " << player->defaultOutfit.lookType << ',';
	query << "`maglevel` = " << player->magLevel << ',';
	query << "`mana` = " << player->mana << ',';
	query << "`manamax` = " << player->manaMax << ',';
	query << "`manaspent` = " << player->manaSpent << ',';
	query << "`soul` = " << static_cast<uint16_t>(player->soul) << ',';
	query << "`town_id` = " << player->town->getID() << ',';
	query << "`sex` = " << static_cast<uint16_t>(player->sex) << ',';
	query << "`posx` = " << player->position.x << ',';
	query << "`posy` = " << player->position.y << ',';
	query << "`posz` = " << static_cast<uint32_t>(player->position.z) << ',';
	query << "`cap` = " << player->capacity << ',';
	query << "`blessings` = " << player->blessings.to_ulong() << ',';

	if (player->lastLoginSaved != 0) {
		query << "`lastlogin` = " << player->lastLoginSaved << ',';
	}

	if (player->lastIP != 0) {
		query << "`lastip` = " << player->lastIP << ',';
	}

	if (g_game.getWorldType() != WORLD_TYPE_PVP_ENFORCED) {
		query << "`skulltime` = " << player->playerKillerEnd << ',';

		Skulls_t skull = SKULL_NONE;
		if (player->skull == SKULL_RED) {
			skull = SKULL_RED;
		}

		query << "`skull` = " << static_cast<uint32_t>(skull) << ',';
	}

	query << "`lastlogout` = " << player->getLastLogout() << ',';
	query << "`balance` = " << player->bankBalance << ',';
	query << "`stamina` = " << player->getStaminaMinutes() << ',';

	if (!player->isOffline()) {
		query << "`onlinetime` = `onlinetime` + " << time(nullptr) - player->lastLoginSaved << ',';
	}

	query << "`skill_fist` = " << player->skills[SKILL_FIST].level << ',';
	query << "`skill_fist_tries` = " << player->skills[SKILL_FIST].tries << ',';
	query << "`skill_club` = " << player->skills[SKILL_CLUB].level << ',';
	query << "`skill_club_tries` = " << player->skills[SKILL_CLUB].tries << ',';
	query << "`skill_sword` = " << player->skills[SKILL_SWORD].level << ',';
	query << "`skill_sword_tries` = " << player->skills[SKILL_SWORD].tries << ',';
	query << "`skill_axe` = " << player->skills[SKILL_AXE].level << ',';
	query << "`skill_axe_tries` = " << player->skills[SKILL_AXE].tries << ',';
	query << "`skill_dist` = " << player->skills[SKILL_DISTANCE].level << ',';
	query << "`skill_dist_tries` = " << player->skills[SKILL_DISTANCE].tries << ',';
	query << "`skill_shielding` = " << player->skills[SKILL_SHIELD].level << ',';
	query << "`skill_shielding_tries` = " << player->skills[SKILL_SHIELD].tries << ',';
	query << "`skill_fishing` = " << player->skills[SKILL_FISHING].level << ',';
	query << "`skill_fishing_tries` = " << player->skills[SKILL_FISHING].tries << ',';
	query << "`conditions` = " << db.escapeBlob(condData, condSize);

	std::ostringstream unjustsStream;
	for (auto it = player->murderTimeStamps.begin(); it != player->murderTimeStamps.end(); ++it) {
		if (it != player->murderTimeStamps.begin()) {
			unjustsStream << ',';
		}
		unjustsStream << *it;
	}
	query << ", `unjusts` = " << db.escapeString(unjustsStream.str());

	query << " WHERE `id` = " << player->getGUID();

	if (!db.executeQuery(query.str())) {
		return false;
	}

	// Save inventory items
	if (!db.executeQuery(fmt::format("DELETE FROM `player_items` WHERE `player_id` = {:d}", player->getGUID()))) {
		return false;
	}

	DBInsert itemsInsert("INSERT INTO `player_items` (`player_id`, `pid`, `sid`, `itemtype`, `count`, `attributes`) VALUES ");
	int32_t slotId = 0;
	for (int32_t slot = CONST_SLOT_FIRST; slot <= CONST_SLOT_LAST; slot++) {
		Item* item = player->inventory[slot];
		if (!item) {
			continue;
		}

		PropWriteStream propWriteStream;
		item->serializeTVPFormat(propWriteStream);
		size_t attrSize;
		const char* attrData = propWriteStream.getStream(attrSize);

		query.str(std::string());
		query << player->getGUID() << ',' << slot << ',' << slotId++ << ',' << item->getID() << ',' << item->getItemCount() << ',' << db.escapeBlob(attrData, attrSize);
		if (!itemsInsert.addRow(query)) {
			return false;
		}
	}
	if (!itemsInsert.execute()) {
		return false;
	}

	// Save storage values
	if (!db.executeQuery(fmt::format("DELETE FROM `player_storage` WHERE `player_id` = {:d}", player->getGUID()))) {
		return false;
	}

	DBInsert storageInsert("INSERT INTO `player_storage` (`player_id`, `key`, `value`) VALUES ");
	for (const auto& [key, value] : player->storageMap) {
		query.str(std::string());
		query << player->getGUID() << ',' << key << ',' << value;
		if (!storageInsert.addRow(query)) {
			return false;
		}
	}
	if (!storageInsert.execute()) {
		return false;
	}

	// Save string storage values
	if (!db.executeQuery(fmt::format("DELETE FROM `player_string_storage` WHERE `player_id` = {:d}", player->getGUID()))) {
		return false;
	}

	DBInsert stringStorageInsert("INSERT INTO `player_string_storage` (`player_id`, `key`, `value`) VALUES ");
	for (const auto& [key, value] : player->stringStorageMap) {
		query.str(std::string());
		query << player->getGUID() << ',' << db.escapeString(key) << ',' << db.escapeString(value);
		if (!stringStorageInsert.addRow(query)) {
			return false;
		}
	}
	if (!stringStorageInsert.execute()) {
		return false;
	}

	// Save spells
	if (!db.executeQuery(fmt::format("DELETE FROM `player_spells` WHERE `player_id` = {:d}", player->getGUID()))) {
		return false;
	}

	DBInsert spellsInsert("INSERT INTO `player_spells` (`player_id`, `name`) VALUES ");
	for (const std::string& spellName : player->learnedInstantSpellList) {
		query.str(std::string());
		query << player->getGUID() << ',' << db.escapeString(spellName);
		if (!spellsInsert.addRow(query)) {
			return false;
		}
	}
	if (!spellsInsert.execute()) {
		return false;
	}

	// Save VIP list
	if (!db.executeQuery(fmt::format("DELETE FROM `player_viplist` WHERE `player_id` = {:d}", player->getGUID()))) {
		return false;
	}

	DBInsert vipInsert("INSERT INTO `player_viplist` (`player_id`, `vip_id`) VALUES ");
	for (uint32_t vipId : player->VIPList) {
		query.str(std::string());
		query << player->getGUID() << ',' << vipId;
		if (!vipInsert.addRow(query)) {
			return false;
		}
	}
	if (!vipInsert.execute()) {
		return false;
	}



	// Save depot items
	if (!db.executeQuery(fmt::format("DELETE FROM `player_depotitems` WHERE `player_id` = {:d}", player->getGUID()))) {
		return false;
	}

	for (const auto& [depotId, locker] : player->depotLockerMap) {
		PropWriteStream propWriteStream;
		
		// Write item count first
		propWriteStream.write<uint32_t>(locker->getItemList().size());
		
		// Write each item
		for (int32_t i = locker->getItemList().size() - 1; i >= 0; i--) {
			Item* item = locker->getItemByIndex(i);
			item->serializeTVPFormat(propWriteStream);
		}

		size_t itemsSize;
		const char* itemsData = propWriteStream.getStream(itemsSize);

		if (!db.executeQuery(fmt::format(
			"INSERT INTO `player_depotitems` (`player_id`, `depot_id`, `items`) VALUES ({:d}, {:d}, {:s})",
			player->getGUID(), depotId, db.escapeBlob(itemsData, itemsSize)
		))) {
			return false;
		}
	}



	return transaction.commit();
}


std::string IOLoginData::getNameByGuid(uint32_t guid)
{
	DBResult_ptr result = Database::getInstance().storeQuery(fmt::format("SELECT `name` FROM `players` WHERE `id` = {:d}", guid));
	if (!result) {
		return {};
	}
	return result->getString("name");
}

uint32_t IOLoginData::getGuidByName(const std::string& name)
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery(fmt::format("SELECT `id` FROM `players` WHERE `name` = {:s}", db.escapeString(name)));
	if (!result) {
		return 0;
	}
	return result->getNumber<uint32_t>("id");
}

bool IOLoginData::getGuidByNameEx(uint32_t& guid, bool& specialVip, std::string& name)
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery(fmt::format("SELECT `name`, `id`, `group_id`, `account_id` FROM `players` WHERE `name` = {:s}", db.escapeString(name)));
	if (!result) {
		return false;
	}

	name = result->getString("name");
	guid = result->getNumber<uint32_t>("id");
	Group* group = g_game.groups.getGroup(result->getNumber<uint16_t>("group_id"));

	uint64_t flags;
	if (group) {
		flags = group->flags;
	} else {
		flags = 0;
	}

	specialVip = (flags & PlayerFlag_SpecialVIP) != 0;
	return true;
}

bool IOLoginData::formatPlayerName(std::string& name)
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery(fmt::format("SELECT `name` FROM `players` WHERE `name` = {:s}", db.escapeString(name)));
	if (!result) {
		return false;
	}

	name = result->getString("name");
	return true;
}

void IOLoginData::increaseBankBalance(uint32_t guid, uint64_t bankBalance)
{
	Database::getInstance().executeQuery(fmt::format("UPDATE `players` SET `balance` = `balance` + {:d} WHERE `id` = {:d}", bankBalance, guid));
}

bool IOLoginData::hasBiddedOnHouse(uint32_t guid)
{
	Database& db = Database::getInstance();
	return db.storeQuery(fmt::format("SELECT `id` FROM `houses` WHERE `highest_bidder` = {:d} LIMIT 1", guid)).get() != nullptr;
}

void IOLoginData::updatePremiumTime(uint32_t accountId, time_t endTime)
{
	Database::getInstance().executeQuery(fmt::format("UPDATE `accounts` SET `premium_ends_at` = {:d} WHERE `id` = {:d}", endTime, accountId));
}
