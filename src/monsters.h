// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include "creature.h"

const uint32_t MAX_LOOTCHANCE = 100000;

struct LootBlock
{
	uint16_t id;
	uint32_t countmax;
	uint32_t chance;

	// optional
	int32_t subType;
	int32_t actionId;
	std::string text;

	std::vector<LootBlock> childLoot;
	LootBlock()
	{
		id = 0;
		countmax = 1;
		chance = 0;

		subType = -1;
		actionId = -1;
	}
};

class Loot
{
public:
	Loot() = default;

	// non-copyable
	Loot(const Loot&) = delete;
	Loot& operator=(const Loot&) = delete;

	LootBlock lootBlock;
};

struct summonBlock_t
{
	std::string name;
	uint32_t chance;
	uint32_t delay;
	uint32_t speed;
	uint32_t max;
	bool force = false;
};

class BaseSpell;
struct spellBlock_t
{
	constexpr spellBlock_t() = default;
	~spellBlock_t();
	spellBlock_t(const spellBlock_t& other) = delete;
	spellBlock_t& operator=(const spellBlock_t& other) = delete;
	spellBlock_t(spellBlock_t&& other) :
	    spell(other.spell),
	    chance(other.chance),
	    delay(other.delay),
	    speed(other.speed),
	    range(other.range),
	    minCombatValue(other.minCombatValue),
	    maxCombatValue(other.maxCombatValue),
	    meleeEnergyCondition(other.meleeEnergyCondition),
	    meleeFireCondition(other.meleeFireCondition),
	    meleePoisonCondition(other.meleePoisonCondition),
	    combatSpell(other.combatSpell),
	    isMelee(other.isMelee),
	    updateLook(other.updateLook)
	{
		other.spell = nullptr;
	}

	BaseSpell* spell = nullptr;
	uint32_t chance = 100;
	uint32_t delay = 1;
	uint32_t speed = 2000;
	uint32_t range = 0;
	int32_t minCombatValue = 0;
	int32_t maxCombatValue = 0;
	int32_t meleePoisonCondition = 0;
	int32_t meleeFireCondition = 0;
	int32_t meleeEnergyCondition = 0;
	bool combatSpell = false;
	bool isMelee = false;
	bool updateLook = false;
};

struct voiceBlock_t
{
	std::string text;
	bool yellText;
};

class MonsterType
{
	struct MonsterInfo
	{
		LuaScriptInterface* scriptInterface;

		std::map<CombatType_t, int32_t> elementMap;

		std::vector<voiceBlock_t> voiceVector;

		std::vector<LootBlock> lootItems;
		std::vector<std::string> scripts;
		std::vector<spellBlock_t> attackSpells;
		std::vector<spellBlock_t> defenseSpells;
		std::vector<summonBlock_t> summons;

		Skulls_t skull = SKULL_NONE;
		Outfit_t outfit = {};
		RaceType_t race = RACE_BLOOD;

		LightInfo light = {};
		uint16_t lookcorpse = 0;

		uint64_t experience = 0;

		uint32_t skillFactorPercent = 0;
		uint32_t skillNextLevel = 0;
		uint32_t skillAddCount = 0;
		uint32_t baseAttack = 0;
		uint32_t baseSkill = 0;
		uint32_t manaCost = 0;
		uint32_t yellChance = 0;
		uint32_t yellSpeedTicks = 0;
		uint32_t maxSummons = 0;
		uint32_t changeTargetSpeed = 0;
		uint32_t conditionImmunities = 0;
		uint32_t damageImmunities = 0;
		uint32_t baseSpeed = 200;

		int32_t creatureIdleEvent = -1;
		int32_t creatureAppearEvent = -1;
		int32_t creatureDisappearEvent = -1;
		int32_t creatureMoveEvent = -1;
		int32_t creatureSayEvent = -1;
		int32_t thinkEvent = -1;
		int32_t targetDistance = 1;
		int32_t runAwayHealth = 0;
		int32_t health = 100;
		int32_t healthMax = 100;
		int32_t changeTargetChance = 0;
		int32_t strategyNearestEnemy = 0;
		int32_t strategyWeakestEnemy = 0;
		int32_t strategyMostDamageEnemy = 0;
		int32_t strategyRandomEnemy = 0;
		int32_t defense = 0;
		int32_t armor = 0;

		bool canPushItems = false;
		bool canPushCreatures = false;
		bool pushable = true;
		bool isAttackable = true;
		bool isBoss = false;
		bool isChallengeable = true;
		bool isConvinceable = false;
		bool isIgnoringSpawnBlock = false;
		bool isIllusionable = false;
		bool isSummonable = false;
		bool hiddenHealth = false;
		bool canWalkOnEnergy = false;
		bool canWalkOnFire = false;
		bool canWalkOnPoison = false;

		MonstersEvent_t eventType = MONSTERS_EVENT_NONE;
	};

public:
	MonsterType() = default;

	// non-copyable
	MonsterType(const MonsterType&) = delete;
	MonsterType& operator=(const MonsterType&) = delete;

	bool loadCallback(LuaScriptInterface* scriptInterface);

	std::string name;
	std::string nameDescription;

	MonsterInfo info;

	void loadLoot(MonsterType* monsterType, LootBlock lootBlock);
};

class MonsterSpell
{
public:
	MonsterSpell() = default;

	MonsterSpell(const MonsterSpell&) = delete;
	MonsterSpell& operator=(const MonsterSpell&) = delete;

	std::string name = "";
	std::string scriptName = "";

	uint8_t chance = 100;
	uint8_t delay = 0;
	uint8_t range = 0;
	uint8_t drunkenness = 0;

	uint16_t interval = 2000;

	int32_t minCombatValue = 0;
	int32_t maxCombatValue = 0;
	int32_t attack = 0;
	int32_t skill = 0;
	int32_t length = 0;
	int32_t spread = 0;
	int32_t radius = 0;
	int32_t conditionMinDamage = 0;
	int32_t conditionMaxDamage = 0;
	int32_t conditionStartDamage = 0;
	int32_t tickInterval = 0;
	int32_t speedVariation = 0;
	int32_t speedDelta = 0;
	int32_t duration = 0;

	bool isScripted = false;
	bool needTarget = false;
	bool needDirection = false;
	bool combatSpell = false;
	bool isMelee = false;

	Outfit_t outfit = {};
	ShootType_t shoot = CONST_ANI_NONE;
	MagicEffectClasses effect = CONST_ME_NONE;
	ConditionType_t conditionType = CONDITION_NONE;
	CombatType_t combatType = COMBAT_UNDEFINEDDAMAGE;
};

class Monsters
{
public:
	Monsters() = default;
	// non-copyable
	Monsters(const Monsters&) = delete;
	Monsters& operator=(const Monsters&) = delete;

	bool loadFromXml(bool reloading = false);
	bool isLoaded() const { return loaded; }
	bool reload();

	MonsterType* getMonsterType(const std::string& name, bool loadFromFile = true);
	bool deserializeSpell(MonsterSpell* spell, spellBlock_t& sb, const std::string& description = "");

	std::unique_ptr<LuaScriptInterface> scriptInterface;
	std::map<std::string, MonsterType> monsters;

private:
	ConditionDamage* getDamageCondition(ConditionType_t conditionType, int32_t maxDamage, int32_t minDamage,
	                                    int32_t startDamage, uint32_t tickInterval);
	ConditionDamage* getDamageCondition(ConditionType_t conditionType, int32_t cycle, int32_t count, int32_t maxCount,
	                                    int32_t minCycle = 0);

	bool deserializeSpell(MonsterType* mType, const pugi::xml_node& node, spellBlock_t& sb,
	                      const std::string& description = "");

	MonsterType* loadMonster(const std::string& file, const std::string& monsterName, bool reloading = false);

	void loadLootContainer(const pugi::xml_node& node, LootBlock&);
	bool loadLootItem(const pugi::xml_node& node, LootBlock&);

	std::map<std::string, std::string> unloadedMonsters;

	bool loaded = false;
};
