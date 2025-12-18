// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include <algorithm>
#if __has_include("luajit/lua.hpp")
#include <luajit/lua.hpp>
#else
#include <lua.hpp>
#endif

#include "configmanager.h"
#include "game.h"
#include "monster.h"
#include "pugicast.h"

#if LUA_VERSION_NUM >= 502
#undef lua_strlen
#define lua_strlen lua_rawlen
#endif

extern Game g_game;

namespace {

std::string getGlobalString(lua_State* L, const char* identifier, const char* defaultValue)
{
	lua_getglobal(L, identifier);
	if (!lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return defaultValue;
	}

	size_t len = lua_strlen(L, -1);
	std::string ret(lua_tostring(L, -1), len);
	lua_pop(L, 1);
	return ret;
}

int32_t getGlobalNumber(lua_State* L, const char* identifier, const int32_t defaultValue = 0)
{
	lua_getglobal(L, identifier);
	if (!lua_isnumber(L, -1)) {
		lua_pop(L, 1);
		return defaultValue;
	}

	int32_t val = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return val;
}

bool getGlobalBoolean(lua_State* L, const char* identifier, const bool defaultValue)
{
	lua_getglobal(L, identifier);
	if (!lua_isboolean(L, -1)) {
		if (!lua_isstring(L, -1)) {
			lua_pop(L, 1);
			return defaultValue;
		}

		size_t len = lua_strlen(L, -1);
		std::string ret(lua_tostring(L, -1), len);
		lua_pop(L, 1);
		return booleanString(ret);
	}

	int val = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return val != 0;
}

}

ConfigManager::ConfigManager()
{
	string[CONFIG_FILE] = "config.lua";
}

namespace {

ExperienceStages loadXMLStages()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/XML/stages.xml");
	if (!result) {
		printXMLError("Error - loadXMLStages", "data/XML/stages.xml", result);
		return {};
	}

	ExperienceStages stages;
	for (auto stageNode : doc.child("stages").children()) {
		if (strcasecmp(stageNode.name(), "config") == 0) {
			if (!stageNode.attribute("enabled").as_bool()) {
				return {};
			}
		} else {
			uint32_t minLevel = 1, maxLevel = std::numeric_limits<uint32_t>::max(), multiplier = 1;

			if (auto minLevelAttribute = stageNode.attribute("minlevel")) {
				minLevel = pugi::cast<uint32_t>(minLevelAttribute.value());
			}

			if (auto maxLevelAttribute = stageNode.attribute("maxlevel")) {
				maxLevel = pugi::cast<uint32_t>(maxLevelAttribute.value());
			}

			if (auto multiplierAttribute = stageNode.attribute("multiplier")) {
				multiplier = pugi::cast<uint32_t>(multiplierAttribute.value());
			} else {
				multiplier = 1;
			}

			stages.emplace_back(minLevel, maxLevel, multiplier);
		}
	}

	std::sort(stages.begin(), stages.end());
	return stages;
}

}

bool ConfigManager::load()
{
	lua_State* L = luaL_newstate();
	if (!L) {
		throw std::runtime_error("Failed to allocate memory");
	}

	luaL_openlibs(L);

	if (luaL_dofile(L, getString(CONFIG_FILE).c_str())) {
		std::cout << "[Error - ConfigManager::load] " << lua_tostring(L, -1) << std::endl;
		lua_close(L);
		return false;
	}

	//parse config
	if (!loaded) { //info that must be loaded one time (unless we reset the modules involved)
		boolean[BIND_ONLY_GLOBAL_ADDRESS] = getGlobalBoolean(L, "bindOnlyGlobalAddress", false);
		boolean[OPTIMIZE_DATABASE] = getGlobalBoolean(L, "startupDatabaseOptimization", true);

		if (string[IP] == "") {
			string[IP] = getGlobalString(L, "ip", "127.0.0.1");
		}

		string[MAP_NAME] = getGlobalString(L, "mapName", "world");
		string[MAP_AUTHOR] = getGlobalString(L, "mapAuthor", "Unknown");
		string[HOUSE_RENT_PERIOD] = getGlobalString(L, "houseRentPeriod", "never");
		string[MYSQL_HOST] = getGlobalString(L, "mysqlHost", "127.0.0.1");
		string[MYSQL_USER] = getGlobalString(L, "mysqlUser", "forgottenserver");
		string[MYSQL_PASS] = getGlobalString(L, "mysqlPass", "");
		string[MYSQL_DB] = getGlobalString(L, "mysqlDatabase", "forgottenserver");
		string[MYSQL_SOCK] = getGlobalString(L, "mysqlSock", "");
		string[DISABLED_MAILBOXES] = getGlobalString(L, "disabledMailboxes", "");

		integer[SQL_PORT] = getGlobalNumber(L, "mysqlPort", 3306);

		if (integer[GAME_PORT] == 0) {
			integer[GAME_PORT] = getGlobalNumber(L, "gameProtocolPort", 7172);
		}

		if (integer[LOGIN_PORT] == 0) {
			integer[LOGIN_PORT] = getGlobalNumber(L, "loginProtocolPort", 7171);
		}

		integer[STATUS_PORT] = getGlobalNumber(L, "statusProtocolPort", 7171);

		boolean[ENABLE_MAP_REFRESH] = getGlobalBoolean(L, "enableMapRefresh", true);
		boolean[CLASSIC_MONSTER_INVISIBILITY] = getGlobalBoolean(L, "classicMonsterInvisibility", true);
		boolean[MYSQL_USE_SSL] = getGlobalBoolean(L, "mysqlUseSSL", false);

		integer[MAP_REFRESH_INTERVAL] = getGlobalNumber(L, "mapRefreshInterval", 30 * 1000);
		integer[MAP_REFRESH_TILES_PER_CYCLE] = getGlobalNumber(L, "mapRefreshTilesPerCycle", 32 * 32);
		integer[MAP_REFRESH_VISIBILITY_INTERVAL] = getGlobalNumber(L, "mapRefreshTileVisibilityInterval", 5 * 60 * 1000);

		integer[TILE_ITEM_LIMIT] = getGlobalNumber(L, "tileItemLimit", 1000);
		integer[HOUSE_TILE_ITEM_LIMIT] = getGlobalNumber(L, "houseTileItemLimit", 100);
		integer[MAX_OPEN_CONTAINERS] = getGlobalNumber(L, "maxOpenContainers", 15);
	}

	boolean[ENABLE_MAP_DATA_FILES] = getGlobalBoolean(L, "enableMapDataFiles", true);
	boolean[ALLOW_CHANGEOUTFIT] = getGlobalBoolean(L, "allowChangeOutfit", true);
	boolean[ONE_PLAYER_ON_ACCOUNT] = getGlobalBoolean(L, "onePlayerOnlinePerAccount", true);
	boolean[REMOVE_RUNE_CHARGES] = getGlobalBoolean(L, "removeChargesFromRunes", true);
	boolean[REMOVE_WEAPON_AMMO] = getGlobalBoolean(L, "removeWeaponAmmunition", true);
	boolean[REMOVE_WEAPON_CHARGES] = getGlobalBoolean(L, "removeWeaponCharges", true);
	boolean[REMOVE_POTION_CHARGES] = getGlobalBoolean(L, "removeChargesFromPotions", true);
	boolean[EXPERIENCE_FROM_PLAYERS] = getGlobalBoolean(L, "experienceByKillingPlayers", false);
	boolean[FREE_PREMIUM] = getGlobalBoolean(L, "freePremium", false);
	boolean[REPLACE_KICK_ON_LOGIN] = getGlobalBoolean(L, "replaceKickOnLogin", true);
	boolean[ALLOW_CLONES] = getGlobalBoolean(L, "allowClones", false);
	boolean[EMOTE_SPELLS] = getGlobalBoolean(L, "emoteSpells", false);
	boolean[STAMINA_SYSTEM] = getGlobalBoolean(L, "staminaSystem", true);
	boolean[WARN_UNSAFE_SCRIPTS] = getGlobalBoolean(L, "warnUnsafeScripts", true);
	boolean[CONVERT_UNSAFE_SCRIPTS] = getGlobalBoolean(L, "convertUnsafeScripts", true);
	boolean[SCRIPTS_CONSOLE_LOGS] = getGlobalBoolean(L, "showScriptsLogInConsole", true);
	boolean[YELL_ALLOW_PREMIUM] = getGlobalBoolean(L, "yellAlwaysAllowPremium", false);
	boolean[FORCE_MONSTERTYPE_LOAD] = getGlobalBoolean(L, "forceMonsterTypesOnLoad", true);
	boolean[DEFAULT_WORLD_LIGHT] = getGlobalBoolean(L, "defaultWorldLight", true);
	boolean[HOUSE_OWNED_BY_ACCOUNT] = getGlobalBoolean(L, "houseOwnedByAccount", false);
	boolean[LUA_ITEM_DESC] = getGlobalBoolean(L, "luaItemDesc", false);
	boolean[HOUSE_DOOR_SHOW_PRICE] = getGlobalBoolean(L, "houseDoorShowPrice", true);
	boolean[ONLY_INVITED_CAN_MOVE_HOUSE_ITEMS] = getGlobalBoolean(L, "onlyInvitedCanMoveHouseItems", true);
	boolean[ONLY_INVITED_CAN_ADD_HOUSE_ITEMS] = getGlobalBoolean(L, "onlyInvitedCanAddHouseItems", false);
	boolean[REMOVE_ON_DESPAWN] = getGlobalBoolean(L, "removeOnDespawn", true);
	boolean[PLAYER_CONSOLE_LOGS] = getGlobalBoolean(L, "showPlayerLogInConsole", true);
	boolean[USE_CLASSIC_COMBAT_FORMULAS] = getGlobalBoolean(L, "useClassicCombatFormulas", true);
	boolean[ALLOW_PLAYER_ROOKING] = getGlobalBoolean(L, "allowPlayerRooking", true);
	boolean[ALLOW_UNFAIRFIGHT_DEATH_REDUCTION] = getGlobalBoolean(L, "allowUnfairFightDeathReduction", false);
	boolean[SHOW_MONSTER_LOOT_MESSAGE] = getGlobalBoolean(L, "showMonsterLootMessage", false);
	boolean[CLASSIC_PLAYER_LOOTDROP] = getGlobalBoolean(L, "classicPlayerLootDrop", true);
	boolean[MONSTERS_SPAWN_WITH_LOOT] = getGlobalBoolean(L, "monstersSpawnWithLoot", true);
	boolean[PLAYER_INVENTORY_AUTOSTACK] = getGlobalBoolean(L, "playerInventoryAutoStack", false);
	boolean[ONLY_ONE_FRAG_PER_KILL] = getGlobalBoolean(L, "onlyOneFragPerKill", false);
	boolean[HOUSES_BANKSYSTEM] = getGlobalBoolean(L, "housesBankSystem", false);
	boolean[TRASHABLE_MAILBOX] = getGlobalBoolean(L, "trashableMailbox", false);
	boolean[CLASSIC_INVENTORY_SWAP] = getGlobalBoolean(L, "classicInventorySwap", false);
	boolean[ALLOW_MONSTER_OVERSPAWN] = getGlobalBoolean(L, "allowMonsterOverspawn", true);
	boolean[NEED_LEARN_SPELLS] = getGlobalBoolean(L, "needLearnSpells", true);
	boolean[NO_SPELL_REQUIREMENTS] = getGlobalBoolean(L, "noSpellRequirements", false);
	boolean[UNLIMITED_PLAYER_HP] = getGlobalBoolean(L, "unlimitedPlayerHP", false);
	boolean[UNLIMITED_PLAYER_MP] = getGlobalBoolean(L, "unlimitedPlayerMP", false);
	boolean[DISABLE_MONSTER_SPAWNS] = getGlobalBoolean(L, "disableMonsterSpawns", false);
	boolean[HOUSE_DOORS_DISPLAY_HOUSEINFO] = getGlobalBoolean(L, "houseDoorsDisplayHouseInfo", false);
	boolean[DEEP_PLAYER_CONTAINER_SEARCH] = getGlobalBoolean(L, "deepPlayerContainerSearch", false);
	boolean[GUILHALLS_ONLYFOR_LEADERS] = getGlobalBoolean(L, "guildHallsOnlyForLeaders", false);
	boolean[HOUSES_ONLY_PREMIUM] = getGlobalBoolean(L, "housesOnlyPremium", true);
	boolean[UPON_MAP_UPDATE_SENDPLAYERS_TO_TEMPLE] = getGlobalBoolean(L, "uponMapUpdateSendPlayersToTemple", true);
	boolean[ITEMS_DECAY_INSIDE_DEPOTS] = getGlobalBoolean(L, "itemsDecayInsideDepots", false);
	boolean[HOUSE_TRANSFEROWNERSHIP_TRANSFERITEMS] = getGlobalBoolean(L, "houseTransferItems", false);
	boolean[HOUSE_TRANSFEROWNERSHIP_CLEANBEDS] = getGlobalBoolean(L, "houseCleanBeds", false);
	boolean[HOUSE_TRANSFEROWNERSHIP_CLEARDOORS] = getGlobalBoolean(L, "houseClearDoors", false);
	boolean[ALLOW_RUNEAIMBOT_FROM_DISTANCE] = getGlobalBoolean(L, "allowRuneAimBotFromDistance", true);
	boolean[SPAWN_ALL_MONSTERS_PER_SPAWN] = getGlobalBoolean(L, "spawnAllMonstersPerSpawn", false);
	boolean[GAMEMASTER_DAMAGEPROTECTONZONEEFFECTS] = getGlobalBoolean(L, "gamemasterDamageProtectOnZoneEffects", false);
	boolean[GAME_BEAT_SIMULATION] = getGlobalBoolean(L, "gameBeatSimulation", true);
	boolean[SPAWNS_RESET_TIMER_ON_FAILED_SPAWN] = getGlobalBoolean(L, "spawnsResetTimerOnFailedSpawn", true);
	boolean[SPAWN_ONE_MONSTER_AT_A_TIME] = getGlobalBoolean(L, "spawnOneMonsterAtATime", false);
	boolean[TILE_OLDSCHOOL_ITEM_STACKING] = getGlobalBoolean(L, "tileOldschoolItemStacking", false);
	boolean[SPAWN_MULTIFLOOR_RESPAWN_BLOCK] = getGlobalBoolean(L, "spawnMultifloorRespawnBlock", false);

	string[DEFAULT_PRIORITY] = getGlobalString(L, "defaultPriority", "high");
	string[SERVER_NAME] = getGlobalString(L, "serverName", "");
	string[OWNER_NAME] = getGlobalString(L, "ownerName", "");
	string[OWNER_EMAIL] = getGlobalString(L, "ownerEmail", "");
	string[URL] = getGlobalString(L, "url", "");
	string[LOCATION] = getGlobalString(L, "location", "");
	string[MOTD] = getGlobalString(L, "motd", "");
	string[WORLD_TYPE] = getGlobalString(L, "worldType", "pvp");
	string[ROOK_TOWN_NAME] = getGlobalString(L, "rookTownName", "Rookgaard");
	string[IP_LOCK_MESSAGE] = getGlobalString(L, "ipLockMessage", "IP address blocked for 30 minutes. Please wait.");
	string[ACCOUNT_LOCK_MESSAGE] = getGlobalString(L, "accountLockMessage", "Account disabled for five minutes. Please wait.");
	string[SERVER_SAVE_TIME] = getGlobalString(L, "serverSaveTime", "04:00:00");

	integer[MAX_PLAYERS] = getGlobalNumber(L, "maxPlayers");
	integer[PZ_LOCKED] = getGlobalNumber(L, "pzLocked", 60000);
	integer[RATE_EXPERIENCE] = getGlobalNumber(L, "rateExp", 5);
	integer[RATE_SKILL] = getGlobalNumber(L, "rateSkill", 3);
	integer[RATE_LOOT] = getGlobalNumber(L, "rateLoot", 2);
	integer[RATE_MAGIC] = getGlobalNumber(L, "rateMagic", 3);
	integer[RATE_SPAWN] = getGlobalNumber(L, "rateSpawn", 1);
	integer[HOUSE_PRICE] = getGlobalNumber(L, "housePriceEachSQM", 1000);
	integer[ACTIONS_DELAY_INTERVAL] = getGlobalNumber(L, "timeBetweenActions", 200);
	integer[EX_ACTIONS_DELAY_INTERVAL] = getGlobalNumber(L, "timeBetweenExActions", 1000);
	integer[MAX_MESSAGEBUFFER] = getGlobalNumber(L, "maxMessageBuffer", 2500);
	integer[KICK_AFTER_MINUTES] = getGlobalNumber(L, "kickIdlePlayerAfterMinutes", 15);
	integer[PROTECTION_LEVEL] = getGlobalNumber(L, "protectionLevel", 1);
	integer[DEATH_LOSE_PERCENT] = getGlobalNumber(L, "deathLosePercent", -1);
	integer[STATUSQUERY_TIMEOUT] = getGlobalNumber(L, "statusTimeout", 5000);
	integer[RED_SKULL_DURATION] = getGlobalNumber(L, "redSkullDuration", 30 * 24 * 60 * 60);
	integer[WHITE_SKULL_TIME] = getGlobalNumber(L, "whiteSkullTime", 15 * 60);
	integer[PVP_EXP_FORMULA] = getGlobalNumber(L, "pvpExpFormula", 10);
	integer[YELL_MINIMUM_LEVEL] = getGlobalNumber(L, "yellMinimumLevel", 2);
	integer[VIP_FREE_LIMIT] = getGlobalNumber(L, "vipFreeLimit", 20);
	integer[VIP_PREMIUM_LIMIT] = getGlobalNumber(L, "vipPremiumLimit", 100);
	integer[DEPOT_FREE_LIMIT] = getGlobalNumber(L, "depotFreeLimit", 2000);
	integer[DEPOT_PREMIUM_LIMIT] = getGlobalNumber(L, "depotPremiumLimit", 10000);
	integer[ROOKING_LEVEL] = getGlobalNumber(L, "rookingLevel", 6);
	integer[BAN_DAYS_LENGTH] = getGlobalNumber(L, "banDaysLength", 30);
	integer[KILLS_DAY_RED_SKULL] = getGlobalNumber(L, "killsDayRedSkull", 3);
	integer[KILLS_WEEK_RED_SKULL] = getGlobalNumber(L, "killsWeekRedSkull", 5);
	integer[KILLS_MONTH_RED_SKULL] = getGlobalNumber(L, "killsMonthRedSkull", 10);
	integer[KILLS_DAY_BANISHMENT] = getGlobalNumber(L, "killsDayBanishment", 5);
	integer[KILLS_WEEK_BANISHMENT] = getGlobalNumber(L, "killsWeekBanishment", 8);
	integer[KILLS_MONTH_BANISHMENT] = getGlobalNumber(L, "killsMonthBanishment", 10);
	integer[FAILED_LOGINATTEMPTS_ACCOUNT_LOCK] = getGlobalNumber(L, "failedLoginAttemptsAccountLock", 10);
	integer[FAILED_LOGINATTEMPTS_IP_BAN] = getGlobalNumber(L, "failedLoginAttemptsIPBan", 15);
	integer[ACCOUNT_LOCK_DURATION] = getGlobalNumber(L, "accountLockDuration", 5 * 60 * 1000);
	integer[IP_LOCK_DURATION] = getGlobalNumber(L, "ipLockDuration", 30 * 60 * 1000);

	expStages = loadXMLStages();
	expStages.shrink_to_fit();

	loaded = true;
	lua_close(L);

	return true;
}

bool ConfigManager::reload()
{
	bool result = load();
	if (transformToSHA1(getString(ConfigManager::MOTD)) != g_game.getMotdHash()) {
		g_game.incrementMotdNum();
	}
	return result;
}

static std::string dummyStr;

const std::string& ConfigManager::getString(string_config_t what) const
{
	if (what >= LAST_STRING_CONFIG) {
		std::cout << "[Warning - ConfigManager::getString] Accessing invalid index: " << what << std::endl;
		return dummyStr;
	}
	return string[what];
}

int32_t ConfigManager::getNumber(integer_config_t what) const
{
	if (what >= LAST_INTEGER_CONFIG) {
		std::cout << "[Warning - ConfigManager::getNumber] Accessing invalid index: " << what << std::endl;
		return 0;
	}
	return integer[what];
}

bool ConfigManager::getBoolean(boolean_config_t what) const
{
	if (what >= LAST_BOOLEAN_CONFIG) {
		std::cout << "[Warning - ConfigManager::getBoolean] Accessing invalid index: " << what << std::endl;
		return false;
	}
	return boolean[what];
}

float ConfigManager::getExperienceStage(uint32_t level) const
{
	auto it = std::find_if(expStages.begin(), expStages.end(), [level](auto&& stage) {
		auto&& [minLevel, maxLevel, _] = stage;
		return level >= minLevel && level <= maxLevel;
		});

	if (it == expStages.end()) {
		return getNumber(ConfigManager::RATE_EXPERIENCE);
	}

	return std::get<2>(*it);
}

bool ConfigManager::setString(string_config_t what, const std::string& value)
{
	if (what >= LAST_STRING_CONFIG) {
		std::cout << "[Warning - ConfigManager::setString] Accessing invalid index: " << what << std::endl;
		return false;
	}

	string[what] = value;
	return true;
}

bool ConfigManager::setNumber(integer_config_t what, int32_t value)
{
	if (what >= LAST_INTEGER_CONFIG) {
		std::cout << "[Warning - ConfigManager::setNumber] Accessing invalid index: " << what << std::endl;
		return false;
	}

	integer[what] = value;
	return true;
}

bool ConfigManager::setBoolean(boolean_config_t what, bool value)
{
	if (what >= LAST_BOOLEAN_CONFIG) {
		std::cout << "[Warning - ConfigManager::setBoolean] Accessing invalid index: " << what << std::endl;
		return false;
	}

	boolean[what] = value;
	return true;
}
