// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "iologindata.h"
#include "configmanager.h"
#include "game.h"
#include "scriptwriter.h"

#include <fmt/format.h>
#include <fstream>
#include <filesystem>

#include "databasetasks.h"

extern ConfigManager g_config;
extern Game g_game;

static std::map<std::string, slots_t> stringToSlot = {
    {"head", CONST_SLOT_HEAD},       {"armor", CONST_SLOT_ARMOR},       {"legs", CONST_SLOT_LEGS},
    {"feet", CONST_SLOT_FEET},       {"right", CONST_SLOT_RIGHT},       {"left", CONST_SLOT_LEFT},
    {"amulet", CONST_SLOT_NECKLACE}, {"backpack", CONST_SLOT_BACKPACK}, {"ammo", CONST_SLOT_AMMO},
    {"ring", CONST_SLOT_RING},
};

static std::map<slots_t, std::string> slotToString = {
    {CONST_SLOT_HEAD, "Head"},       {CONST_SLOT_ARMOR, "Armor"},       {CONST_SLOT_LEGS, "Legs"},
    {CONST_SLOT_FEET, "Feet"},       {CONST_SLOT_RIGHT, "Right"},       {CONST_SLOT_LEFT, "Left"},
    {CONST_SLOT_NECKLACE, "Amulet"}, {CONST_SLOT_BACKPACK, "Backpack"}, {CONST_SLOT_RING, "Ring"},
    {CONST_SLOT_AMMO, "Ammo"},
};

Account IOLoginData::loadAccount(uint32_t accno)
{
	Account account;

	DBResult_ptr result = Database::getInstance().storeQuery(
	    fmt::format("SELECT `id`, `password`, `type`, `premium_ends_at` FROM `accounts` WHERE `id` = {:d}", accno));
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

	DBResult_ptr result = db.storeQuery(fmt::format(
	    "SELECT `id`, `password`, `type`, `premium_ends_at` FROM `accounts` WHERE `id` = {:d}", accountNumber));
	if (!result) {
		return false;
	}

	if (transformToSHA1(password) != result->getString("password")) {
		return false;
	}

	account.id = result->getNumber<uint32_t>("id");
	account.accountType = static_cast<AccountType_t>(result->getNumber<int32_t>("type"));
	account.premiumEndsAt = result->getNumber<time_t>("premium_ends_at");

	result = db.storeQuery(fmt::format(
	    "SELECT `name` FROM `players` WHERE `account_id` = {:d} AND `deletion` = 0 ORDER BY `name` ASC", account.id));
	if (result) {
		do {
			account.characters.push_back(result->getString("name"));
		} while (result->next());
	}
	return true;
}

uint32_t IOLoginData::gameworldAuthentication(uint32_t accountNumber, const std::string& password,
                                              std::string& characterName)
{
	Database& db = Database::getInstance();

	DBResult_ptr result =
	    db.storeQuery(fmt::format("SELECT `id`, `password` FROM `accounts` WHERE `id` = {:d}", accountNumber));
	if (!result) {
		return 0;
	}

	if (transformToSHA1(password) != result->getString("password")) {
		return 0;
	}

	uint32_t accountId = result->getNumber<uint32_t>("id");

	result = db.storeQuery(
	    fmt::format("SELECT `name` FROM `players` WHERE `name` = {:s} AND `account_id` = {:d} AND `deletion` = 0",
	                db.escapeString(characterName), accountId));
	if (!result) {
		return 0;
	}

	characterName = result->getString("name");
	return accountId;
}

uint32_t IOLoginData::getAccountIdByPlayerName(const std::string& playerName)
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery(
	    fmt::format("SELECT `account_id` FROM `players` WHERE `name` = {:s}", db.escapeString(playerName)));
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
	DBResult_ptr result =
	    Database::getInstance().storeQuery(fmt::format("SELECT `type` FROM `accounts` WHERE `id` = {:d}", accountId));
	if (!result) {
		return ACCOUNT_TYPE_NORMAL;
	}
	return static_cast<AccountType_t>(result->getNumber<uint16_t>("type"));
}

void IOLoginData::setAccountType(uint32_t accountId, AccountType_t accountType)
{
	Database::getInstance().executeQuery(fmt::format("UPDATE `accounts` SET `type` = {:d} WHERE `id` = {:d}",
	                                                 static_cast<uint16_t>(accountType), accountId));
}

void IOLoginData::updateOnlineStatus(uint32_t guid, bool login)
{
	if (g_config.getBoolean(ConfigManager::ALLOW_CLONES)) {
		return;
	}

	if (login) {
		Database::getInstance().executeQuery(fmt::format("INSERT INTO `players_online` VALUES ({:d})", guid));
	} else {
		Database::getInstance().executeQuery(
		    fmt::format("DELETE FROM `players_online` WHERE `player_id` = {:d}", guid));
	}
}

bool IOLoginData::preloadPlayer(Player* player, const std::string& name)
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery(fmt::format(
	    "SELECT `p`.`id`, `p`.`sex`, `p`.`vocation`, `p`.`town_id`, `p`.`account_id`, `p`.`group_id`, `a`.`type`, `a`.`premium_ends_at` FROM `players` as `p` JOIN `accounts` as `a` ON `a`.`id` = `p`.`account_id` WHERE `p`.`name` = {:s} AND `p`.`deletion` = 0",
	    db.escapeString(name)));
	if (!result) {
		return false;
	}

	Group* group = g_game.groups.getGroup(result->getNumber<uint16_t>("group_id"));
	if (!group) {
		std::cout << "[Error - IOLoginData::preloadPlayer] " << player->name << " has Group ID "
		          << result->getNumber<uint16_t>("group_id") << " which doesn't exist." << std::endl;
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
	DBResult_ptr result =
	    db.storeQuery(fmt::format("SELECT `id`, `name` FROM `players` WHERE `name` = {:s}", db.escapeString(name)));
	if (!result) {
		return false;
	}

	player->name = result->getString("name");
	player->guid = result->getNumber<uint32_t>("id");
	return loadPlayer(player, false);
}

bool IOLoginData::loadPlayer(Player* player, bool initializeScriptFile)
{
	static const std::string basicMalePlayerFilename = "gamedata/players/male.dat";
	static const std::string basicFemalePlayerFilename = "gamedata/players/female.dat";

	// Find the players sub folder by modulus of player GUID and load
	uint32_t modulus = player->getGUID() % 100;
	const std::string foldername = fmt::format("gamedata/players/{:d}", modulus);
	std::string filename = fmt::format("{}/{:d}.tvpp", foldername, player->getGUID());

	std::ifstream fileTest(filename, std::ios::binary);
	if (!fileTest.is_open()) {
		if (!initializeScriptFile) {
			return false;
		}

		if (player->getSex() == PLAYERSEX_FEMALE) {
			filename = basicFemalePlayerFilename;
			fileTest.open(basicFemalePlayerFilename, std::ios::binary);
			if (!fileTest.is_open()) {
				std::cout << "> ERROR: no female.dat file available." << std::endl;
				return false;
			}
		} else {
			filename = basicMalePlayerFilename;
			fileTest.open(basicMalePlayerFilename, std::ios::binary);
			if (!fileTest.is_open()) {
				std::cout << "> ERROR: no male.dat file available." << std::endl;
				return false;
			}
		}
	}

	ScriptReader script;
	if (!script.loadScript(filename, false)) {
		return false;
	}

	while (script.canRead()) {
		script.nextToken();
		if (script.getToken() == TOKEN_ENDOFFILE) {
			break;
		}

		std::string identifier = script.getIdentifier();
		script.readSymbol('=');

		if (identifier == "id") {
			script.readNumber(); // TODO: Do we need this?
		} else if (identifier == "name") {
			script.readString(); // TODO: Do we need this?
		} else if (identifier == "sex") {
			player->setSex(static_cast<PlayerSex_t>(script.readNumber()));
		} else if (identifier == "vocation") {
			if (!player->setVocation(script.readNumber())) {
				script.error("invalid vocation ID");
				return false;
			}
		} else if (identifier == "town") {
			Town* town = g_game.map.towns.getTown(script.readNumber());
			if (!town) {
				script.error("unknown town");
				return false;
			}

			if (!player->town) {
				// prioritize town obtained from the AAC/database
				player->setTown(town);
			}
		} else if (identifier == "skull") {
			player->setSkull(static_cast<Skulls_t>(script.readNumber()));
		} else if (identifier == "playerkillerend") {
			player->playerKillerEnd = script.readNumber();
		} else if (identifier == "bankbalance") {
			player->bankBalance = script.readNumber();
		} else if (identifier == "blessings") {
			player->blessings = script.readNumber();
		} else if (identifier == "lastloginsaved") {
			player->lastLoginSaved = script.readNumber();
		} else if (identifier == "lastlogout") {
			player->lastLogout = script.readNumber();
		} else if (identifier == "position") {
			player->position = script.readPosition();
			player->loginPosition = player->position;
		} else if (identifier == "defaultoutfit") {
			script.readSymbol('(');
			player->defaultOutfit.lookType = script.readNumber();
			script.readSymbol(',');
			player->defaultOutfit.lookHead = script.readNumber();
			script.readSymbol('-');
			player->defaultOutfit.lookBody = script.readNumber();
			script.readSymbol('-');
			player->defaultOutfit.lookLegs = script.readNumber();
			script.readSymbol('-');
			player->defaultOutfit.lookFeet = script.readNumber();
			script.readSymbol(')');
			player->currentOutfit = player->defaultOutfit;
		} else if (identifier == "level") {
			player->level = script.readNumber();
			player->updateBaseSpeed();
		} else if (identifier == "experience") {
			player->experience = script.readNumber();

			const auto expForLevel = Player::getExpForLevel(player->level);
			if (player->experience < expForLevel) {
				player->experience = expForLevel;
			} else if (player->experience > Player::getExpForLevel(player->level + 1)) {
				player->experience = expForLevel;
			}
		} else if (identifier == "health") {
			player->health = script.readNumber();
		} else if (identifier == "maxhealth") {
			player->healthMax = script.readNumber();
		} else if (identifier == "mana") {
			player->mana = script.readNumber();
		} else if (identifier == "maxmana") {
			player->manaMax = script.readNumber();
		} else if (identifier == "magiclevel") {
			player->magLevel = script.readNumber();
		} else if (identifier == "manaspent") {
			player->manaSpent = script.readNumber();
		} else if (identifier == "soul") {
			player->soul = script.readNumber();
		} else if (identifier == "capacity") {
			player->capacity = script.readNumber();
		} else if (identifier == "stamina") {
			player->staminaMinutes = script.readNumber();
		} else if (identifier == "group") {
			const auto groupID = script.readNumber();
			Group* group = g_game.groups.getGroup(groupID);
			if (!group) {
				std::cout << "[Error - IOLoginData::loadPlayer] " << player->name << " has Group ID " << groupID
				          << " which doesn't exist." << std::endl;
				return false;
			}

			if (!player->group) {
				// prioritize group obtained from the AAC/database
				player->setGroup(group);
			}
		} else if (identifier == "skill") {
			script.readSymbol('(');
			skills_t skill = static_cast<skills_t>(script.readNumber());
			script.readSymbol(',');
			player->skills[skill].level = script.readNumber();
			script.readSymbol(',');
			player->skills[skill].tries = script.readNumber();
			script.readSymbol(')');
		} else if (identifier == "condition") {
			script.readSymbol('(');
			ConditionType_t type = static_cast<ConditionType_t>(script.readNumber());
			script.readSymbol(',');
			Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, type, 0);
			if (!condition->unserializeTVPFormat(script)) {
				delete condition;
				script.error("failed to load condition");
				return false;
			}
			script.readSymbol(')');

			// never load in-fight condition
			if (type == CONDITION_INFIGHT) {
				delete condition;
			} else {
				player->storedConditionList.push_front(condition);
			}
		} else if (identifier == "spells") {
			script.readSymbol('{');
			while (script.canRead()) {
				script.nextToken();
				if (script.getToken() == TOKEN_STRING) {
					player->learnedInstantSpellList.push_front(script.getString());
				} else if (script.getSpecial() == ',') {
					continue;
				} else if (script.getSpecial() != '}') {
					script.error("',' or '}' expected");
					return false;
				} else {
					break;
				}
			}
		} else if (identifier == "questvalues") {
			script.readSymbol('{');
			while (script.canRead()) {
				script.nextToken();
				if (script.getToken() != TOKEN_SPECIAL) {
					script.error("quest-value expected");
					return false;
				}

				if (script.getSpecial() == '}') {
					break;
				} else if (script.getSpecial() == ',') {
					continue;
				} else if (script.getSpecial() == '(') {
					int64_t storageValue = script.readNumber();
					script.readSymbol(',');
					int64_t value = script.readNumber();
					player->storageMap[storageValue] = value;
					script.readSymbol(')');
				} else {
					script.error("quest-value expected");
					return false;
				}
			}
		} else if (identifier == "stringquestvalues") {
			script.readSymbol('{');
			while (script.canRead()) {
				script.nextToken();
				if (script.getToken() != TOKEN_SPECIAL) {
					script.error("quest-value expected");
					return false;
				}

				if (script.getSpecial() == '}') {
					break;
				} else if (script.getSpecial() == ',') {
					continue;
				} else if (script.getSpecial() == '(') {
					std::string storageValue = script.readString();
					script.readSymbol(',');
					std::string value = script.readString();
					player->stringStorageMap[storageValue] = value;
					script.readSymbol(')');
				} else {
					script.error("quest-value expected");
					return false;
				}
			}
		} else if (identifier == "murders") {
			script.readSymbol('{');
			while (script.canRead()) {
				script.nextToken();
				if (script.getToken() == TOKEN_NUMBER) {
					time_t timestamp = static_cast<time_t>(script.getNumber());
					player->murderTimeStamps.push_back(timestamp);
				} else if (script.getSpecial() == ',') {
					continue;
				} else if (script.getSpecial() == '}') {
					break;
				} else {
					script.error("',' or '}' expected");
					return false;
				}
			}
		} else if (identifier == "vip") {
			script.readSymbol('(');
			while (script.canRead()) {
				script.nextToken();
				if (script.getToken() == TOKEN_NUMBER) {
					uint32_t vipID = script.getNumber();
					player->VIPList.insert(vipID);
				} else if (script.getSpecial() == ',') {
					continue;
				} else if (script.getSpecial() == ')') {
					break;
				} else {
					script.error("',' or ')' expected");
					return false;
				}
			}
		} else if (identifier == "depot") {
			script.readSymbol('(');
			uint32_t depotId = script.readNumber();
			script.readSymbol(',');

			DepotLocker* depot = player->getDepotLocker(depotId, true);

			script.readSymbol('{');
			script.nextToken();
			while (script.canRead()) {
				if (script.getToken() == TOKEN_NUMBER) {
					Item* item = Item::CreateItem(script);
					if (!item) {
						script.error("could not create depot item");
						return false;
					}

					if (!item->unserializeTVPFormat(script)) {
						script.error("could not parse item attributes");
						return false;
					}

					depot->internalAddThing(item);
				} else if (script.getSpecial() == ',') {
					script.nextToken();
					continue;
				} else {
					break;
				}
			}
			script.readSymbol(')'); // end of depot
		} else {
			auto slotPtr = stringToSlot.find(identifier);
			if (slotPtr != stringToSlot.end()) {
				slots_t slot = slotPtr->second;

				script.readSymbol('(');
				script.nextToken();
				Item* item = Item::CreateItem(script);
				if (!item) {
					script.error("could not create SLOT item");
					return false;
				}

				if (!item->unserializeTVPFormat(script)) {
					script.error("could not deserialize item data");
					return false;
				}

				player->internalAddThing(slot, item);
				item->startDecaying();

				if (script.getSpecial() != ')') {
					script.error("')' expected");
					return false;
				}

				continue;
			}

			script.error(fmt::format("unknown identifier '{:s}'", identifier));
			return false;
		}
	} // End script-data loading

	std::vector<uint32_t> invalidVIPEntries;
	for (const uint32_t& vip : player->VIPList) {
		if (DBResult_ptr result = Database::getInstance().storeQuery(
		        fmt::format("SELECT `name` FROM `players` WHERE `id` = {:d}", vip))) {
			g_game.storePlayerName(vip, result->getString("name"));
		} else {
			// Player no longer exists
			invalidVIPEntries.push_back(vip);
		}
	}

	// Clean deleted players from the VIP list
	for (const uint32_t& vip : invalidVIPEntries) {
		player->VIPList.erase(vip);
	}

	Database& db = Database::getInstance();
	if (DBResult_ptr result = db.storeQuery(
	        fmt::format("SELECT `guild_id`, `rank_id`, `nick` FROM `guild_membership` WHERE `player_id` = {:d}",
	                    player->getGUID()))) {
		uint32_t guildId = result->getNumber<uint32_t>("guild_id");
		uint32_t playerRankId = result->getNumber<uint32_t>("rank_id");
		player->guildNick = result->getString("nick");

		Guild* guild = g_game.getGuild(guildId);
		if (!guild) {
			guild = IOGuild::loadGuild(guildId);
			if (guild) {
				g_game.addGuild(guild);
			} else {
				std::cout << "[Warning - IOLoginData::loadPlayer] " << player->name << " has Guild ID " << guildId
				          << " which doesn't exist" << std::endl;
			}
		}

		if (guild) {
			player->guild = guild;
			GuildRank_ptr rank = guild->getRankById(playerRankId);
			if (!rank) {
				if ((result = db.storeQuery(fmt::format(
				         "SELECT `id`, `name`, `level` FROM `guild_ranks` WHERE `id` = {:d}", playerRankId)))) {
					guild->addRank(result->getNumber<uint32_t>("id"), result->getString("name"),
					               result->getNumber<uint16_t>("level"));
				}

				rank = guild->getRankById(playerRankId);
				if (!rank) {
					player->guild = nullptr;
				}
			}

			player->guildRank = rank;

			IOGuild::getWarList(guildId, player->guildWarVector);

			if ((result = db.storeQuery(fmt::format(
			         "SELECT COUNT(*) AS `members` FROM `guild_membership` WHERE `guild_id` = {:d}", guildId)))) {
				guild->setMemberCount(result->getNumber<uint32_t>("members"));
			}
		}
	}

	//-- Calculate skills percentages to the next level --//
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
	uint32_t modulus = player->getGUID() % 100;
	const std::string foldername = fmt::format("gamedata/players/{:d}", modulus);
	const std::string filename = fmt::format("{}/{:d}.tvpp", foldername, player->getGUID());

	// Create the required sub folder for us
	if (!std::filesystem::exists(foldername) && !std::filesystem::create_directories(foldername)) {
		std::cout << "> ERROR - [IOLoginData::savePlayer]: Cannot create " << foldername << "." << std::endl;
	}

	ScriptWriter script;
	if (!script.open(filename)) {
		return false;
	}

	script.writeLine("# The Violet Project");
	script.writeLine(fmt::format("# {:s}: player data file", player->getName()));
	script.writeLine();
	script.writeLine(fmt::format("ID = {:d}", player->getGUID()));
	script.writeLine(fmt::format("Name = \"{:s}\"", player->getName()));
	script.writeLine(fmt::format("Town = {:d}", player->getTown()->getID()));
	script.writeLine(fmt::format("Group = {:d}", player->group->id));
	script.writeLine(fmt::format("Skull = {:d}", static_cast<int32_t>(player->getSkull())));
	script.writeLine(fmt::format("Sex = {:d}", static_cast<int32_t>(player->getSex())));
	script.writeLine(fmt::format("PlayerKillerEnd = {:d}", player->playerKillerEnd));
	script.writeLine(fmt::format("BankBalance = {:d}", player->bankBalance));
	script.writeLine(fmt::format("Blessings = {:d}", player->blessings.to_ulong()));
	script.writeLine(fmt::format("LastLoginSaved = {:d}", player->lastLoginSaved));
	script.writeLine(fmt::format("LastLogout = {:d}", player->lastLogout));
	script.writeLine(fmt::format("Position = [{:d},{:d},{:d}]", player->loginPosition.x, player->loginPosition.y,
	                             static_cast<int32_t>(player->loginPosition.z)));
	script.writeLine(fmt::format("DefaultOutfit = ({:d}, {:d}-{:d}-{:d}-{:d})", player->getDefaultOutfit().lookType,
	                             player->getDefaultOutfit().lookHead, player->getDefaultOutfit().lookBody,
	                             player->getDefaultOutfit().lookLegs, player->getDefaultOutfit().lookFeet));
	script.writeLine();
	script.writeLine(fmt::format("Level = {:d}", player->level));
	script.writeLine(fmt::format("Experience = {:d}", player->experience));
	script.writeLine(fmt::format("Health = {:d}", player->health));
	script.writeLine(fmt::format("MaxHealth = {:d}", player->healthMax));
	script.writeLine(fmt::format("Mana = {:d}", player->mana));
	script.writeLine(fmt::format("MaxMana = {:d}", player->manaMax));
	script.writeLine(fmt::format("ManaSpent = {:d}", player->manaSpent));
	script.writeLine(fmt::format("MagicLevel = {:d}", player->magLevel));
	script.writeLine(fmt::format("Soul = {:d}", player->soul));
	script.writeLine(fmt::format("Capacity = {:d}", player->capacity));
	script.writeLine(fmt::format("Vocation = {:d}", player->vocation->getId()));
	script.writeLine(fmt::format("Stamina = {:d}", player->staminaMinutes));
	script.writeLine();
	script.writeLine(fmt::format("Skill = ({:d}, {:d}, {:d})", static_cast<int32_t>(SKILL_FIST),
	                             player->skills[SKILL_FIST].level, player->skills[SKILL_FIST].tries));
	script.writeLine(fmt::format("Skill = ({:d}, {:d}, {:d})", static_cast<int32_t>(SKILL_SWORD),
	                             player->skills[SKILL_SWORD].level, player->skills[SKILL_SWORD].tries));
	script.writeLine(fmt::format("Skill = ({:d}, {:d}, {:d})", static_cast<int32_t>(SKILL_CLUB),
	                             player->skills[SKILL_CLUB].level, player->skills[SKILL_CLUB].tries));
	script.writeLine(fmt::format("Skill = ({:d}, {:d}, {:d})", static_cast<int32_t>(SKILL_AXE),
	                             player->skills[SKILL_AXE].level, player->skills[SKILL_AXE].tries));
	script.writeLine(fmt::format("Skill = ({:d}, {:d}, {:d})", static_cast<int32_t>(SKILL_DISTANCE),
	                             player->skills[SKILL_DISTANCE].level, player->skills[SKILL_DISTANCE].tries));
	script.writeLine(fmt::format("Skill = ({:d}, {:d}, {:d})", static_cast<int32_t>(SKILL_SHIELD),
	                             player->skills[SKILL_SHIELD].level, player->skills[SKILL_SHIELD].tries));
	script.writeLine(fmt::format("Skill = ({:d}, {:d}, {:d})", static_cast<int32_t>(SKILL_FISHING),
	                             player->skills[SKILL_FISHING].level, player->skills[SKILL_FISHING].tries));
	script.writeLine();
	for (Condition* condition : player->conditions) {
		condition->serializeTVPFormat(script);
		script.writeText(")");
		script.writeLine();
	}
	if (!player->storedConditionList.empty()) {
		script.writeLine();
		for (Condition* condition : player->storedConditionList) {
			condition->serializeTVPFormat(script);
			script.writeText(")");
			script.writeLine();
		}
	}
	script.writeText("Spells = {");
	int32_t i = 0;
	for (auto it = player->learnedInstantSpellList.begin(); it != player->learnedInstantSpellList.end(); ++it) {
		script.writeString(*it);
		if (i < player->learnedInstantSpellList.size() - 1) {
			script.writeText(",");
		}
		i++;
	}
	script.writeText("}");
	script.writeLine();
	script.writeLine();
	script.writeText("QuestValues = {");
	i = 0;
	for (auto it = player->storageMap.begin(); it != player->storageMap.end(); ++it) {
		script.writeText(fmt::format("({:d},{:d})", it->first, it->second));
		if (i < player->storageMap.size() - 1) {
			script.writeText(",");
		}
		i++;
	}
	script.writeText("}");
	script.writeLine();
	script.writeLine();
	script.writeText("StringQuestValues = {");
	i = 0;
	for (auto it = player->stringStorageMap.begin(); it != player->stringStorageMap.end(); ++it) {
		script.writeText(fmt::format("(\"{:s}\",\"{:s}\")", it->first, it->second));
		if (i < player->storageMap.size() - 1) {
			script.writeText(",");
		}
		i++;
	}
	script.writeText("}");
	script.writeLine();
	script.writeLine();
	script.writeText("Murders = {");
	i = 0;
	for (auto it = player->murderTimeStamps.begin(); it != player->murderTimeStamps.end(); ++it) {
		script.writeText(fmt::format("{:d}", *it));
		if (i < player->murderTimeStamps.size() - 1) {
			script.writeText(",");
		}
		i++;
	}
	script.writeText("}");
	script.writeLine();
	script.writeLine();
	script.writeText("VIP = (");
	i = 0;
	for (auto it = player->VIPList.begin(); it != player->VIPList.end(); ++it) {
		script.writeText(fmt::format("{:d}", *it));
		if (i < player->VIPList.size() - 1) {
			script.writeText(",");
		}
		i++;
	}
	script.writeText(")");
	script.writeLine();
	script.writeLine();
	for (int32_t slot = CONST_SLOT_FIRST; slot <= CONST_SLOT_LAST; slot++) {
		Item* item = player->inventory[slot];
		if (!item) {
			continue;
		}

		const std::string_view& str = slotToString.find(static_cast<slots_t>(slot))->second;

		script.writeText(fmt::format("{:s} = (", str));
		item->serializeTVPFormat(script);
		script.writeText(")");
		script.writeLine();
	}
	script.writeLine();
	script.writeLine();
	for (const auto& it : player->depotLockerMap) {
		script.writeText("Depot = (");
		script.writeNumber(it.first);
		script.writeText(", {");

		DepotLocker_ptr locker = it.second;
		for (i = locker->getItemList().size() - 1; i >= 0; i--) {
			Item* item = locker->getItemByIndex(i);
			item->serializeTVPFormat(script);

			if (i != 0) {
				script.writeText(", ");
			}
		}
		script.writeText("})");
		script.writeLine();
	}
	script.close();

	// Last step, update SQL specific data, this has to come last in case the SQL server is down
	Database& db = Database::getInstance();
	std::ostringstream query;
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
	query << "`posx` = " << player->getPosition().getX() << ',';
	query << "`posy` = " << player->getPosition().getY() << ',';
	query << "`posz` = " << player->getPosition().getZ() << ',';

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
	query << "`skill_fishing_tries` = " << player->skills[SKILL_FISHING].tries;

	query << " WHERE `id` = " << player->getGUID();

	// We do not care about the result, async query it.
	g_databaseTasks.addTask(query.str());
	g_databaseTasks.addTask(fmt::format("DELETE FROM `player_items` WHERE `player_id` = {:d}", player->getGUID()));

	int32_t inventoryID = CONST_SLOT_HEAD;
	for (auto& inventoryItem : player->inventory) {
		if (!inventoryItem) {
			continue;
		}
		g_databaseTasks.addTask(fmt::format(
		    "INSERT INTO `player_items` (`player_id`, `pid`, `sid`, `itemtype`, `count`) VALUES ({:d}, {:d}, {:d}, {:d}, {:d})",
		    player->getGUID(), inventoryID++, 0, inventoryItem->getID(), inventoryItem->getItemCount()));
	}

	return true;
}

std::string IOLoginData::getNameByGuid(uint32_t guid)
{
	DBResult_ptr result =
	    Database::getInstance().storeQuery(fmt::format("SELECT `name` FROM `players` WHERE `id` = {:d}", guid));
	if (!result) {
		return {};
	}
	return result->getString("name");
}

uint32_t IOLoginData::getGuidByName(const std::string& name)
{
	Database& db = Database::getInstance();

	DBResult_ptr result =
	    db.storeQuery(fmt::format("SELECT `id` FROM `players` WHERE `name` = {:s}", db.escapeString(name)));
	if (!result) {
		return 0;
	}
	return result->getNumber<uint32_t>("id");
}

bool IOLoginData::getGuidByNameEx(uint32_t& guid, bool& specialVip, std::string& name)
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery(fmt::format(
	    "SELECT `name`, `id`, `group_id`, `account_id` FROM `players` WHERE `name` = {:s}", db.escapeString(name)));
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

	DBResult_ptr result =
	    db.storeQuery(fmt::format("SELECT `name` FROM `players` WHERE `name` = {:s}", db.escapeString(name)));
	if (!result) {
		return false;
	}

	name = result->getString("name");
	return true;
}

void IOLoginData::increaseBankBalance(uint32_t guid, uint64_t bankBalance)
{
	Database::getInstance().executeQuery(
	    fmt::format("UPDATE `players` SET `balance` = `balance` + {:d} WHERE `id` = {:d}", bankBalance, guid));
}

bool IOLoginData::hasBiddedOnHouse(uint32_t guid)
{
	Database& db = Database::getInstance();
	return db.storeQuery(fmt::format("SELECT `id` FROM `houses` WHERE `highest_bidder` = {:d} LIMIT 1", guid)).get() !=
	       nullptr;
}

void IOLoginData::updatePremiumTime(uint32_t accountId, time_t endTime)
{
	Database::getInstance().executeQuery(
	    fmt::format("UPDATE `accounts` SET `premium_ends_at` = {:d} WHERE `id` = {:d}", endTime, accountId));
}
