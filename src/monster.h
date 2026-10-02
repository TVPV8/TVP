// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include "tile.h"
#include "monsters.h"
#include "raids.h"

class Creature;
class Game;
class BaseSpawn;

using CreatureHashSet = std::unordered_set<Creature*>;
using CreatureList = std::list<Creature*>;

enum TargetSearchType_t
{
	TARGETSEARCH_NONE,
	TARGETSEARCH_RANDOM,
	TARGETSEARCH_NEAREST,
	TARGETSEARCH_WEAKEST,
	TARGETSEARCH_MOSTDAMAGE,
};

enum STATE : int32_t
{
	SLEEPING = 0,
	IDLE = 1,
	UNDERATTACK = 2,
	TALKING = 3,
	LEAVING = 4,
	ATTACKING = 5,
	PANIC = 6,
};

class Monster final : public Creature
{
public:
	static Monster* createMonster(const std::string& name, const std::vector<LootBlock>* extraLoot = nullptr);

	explicit Monster(MonsterType* mType, const std::vector<LootBlock>* extraLoot = nullptr);
	~Monster();

	// non-copyable
	Monster(const Monster&) = delete;
	Monster& operator=(const Monster&) = delete;

	Monster* getMonster() override { return this; }
	const Monster* getMonster() const override { return this; }

	void setID() override
	{
		if (id == 0) {
			id = monsterAutoID++;
		}
	}

	void addList() override;
	void removeList() override;

	const std::string& getName() const override;
	void setName(const std::string& newName);

	const std::string& getNameDescription() const override;
	void setNameDescription(const std::string& newNameDescription) { nameDescription = newNameDescription; }

	std::string getDescription(int32_t) const override { return nameDescription + '.'; }

	CreatureType_t getType() const override { return CREATURETYPE_MONSTER; }

	const Position& getMasterPos() const { return masterPos; }
	void setMasterPos(Position pos) { masterPos = pos; }

	void setSpawnInterval(uint32_t interval) { spawnInterval = interval; }

	void setLifeTimeExpiration(uint64_t lifetime) { lifeTimeExpiration = lifetime; }

	RaceType_t getRace() const override { return mType->info.race; }
	int32_t getArmor() const override;
	int32_t getDefense() const override;
	bool isPushable() const override { return mType->info.pushable && baseSpeed != 0; }
	bool isAttackable() const override { return mType->info.isAttackable; }

	STATE getState() const { return State; }

	bool canPushItems() const;
	bool canPushCreatures() const { return mType->info.canPushCreatures && !isSummon(); }
	bool isHostile() const { return mType->info.baseSkill != 0 && health > mType->info.runAwayHealth; }
	bool canSee(const Position& pos) const override;
	bool canSeeInvisibility() const override { return isImmune(CONDITION_INVISIBLE); }
	uint32_t getManaCost() const { return mType->info.manaCost; }
	void setSpawn(BaseSpawn* newSpawn) { spawn = newSpawn; }
	bool canWalkOnFieldType(CombatType_t combatType) const;

	void onWalk(Direction& dir, uint32_t& flags) override;

	void onCreatureAppear(Creature* creature, bool isLogin) override;
	void onRemoveCreature(Creature* creature, bool isLogout) override;
	void onCreatureMove(Creature* creature, const Tile* newTile, const Position& newPos, const Tile* oldTile,
	                    const Position& oldPos, bool teleport) override;
	void onCreatureSay(Creature* creature, SpeakClasses type, const std::string& text) override;

	void drainHealth(Creature* attacker, int32_t damage) override;
	void changeHealth(int32_t healthChange, bool sendHealthChange = true) override;

	LightInfo getCreatureLight() const override;

	void onIdleStimulus() override;
	void onThink(uint32_t interval) override;

	bool challengeCreature(Creature* creature, bool force = false) override;

	bool getCombatValues(int32_t& min, int32_t& max) override;

	void doAttackSpells();
	void doDefensiveSpells();
	void spawnSummons();
	void checkVoices();

	void doAttacking() override;

	bool selectTarget(Creature* creature);

	bool isTarget(const Creature* creature) const;
	bool isFleeing() const { return !isSummon() && getHealth() <= mType->info.runAwayHealth; }

	bool getRandomStep(const Position& creaturePos, Direction& resultDir) const;
	bool getFlightStep(const Position& targetPos, Direction& resultDir) const;

	bool isPathBlockingChecking() const { return pathBlockCheck; }

	bool isOpponent(const Creature* creature) const;
	bool isCreatureAvoidable(const Creature* creature) const;

	BlockType_t blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage, bool checkDefense = false,
	                     bool checkArmor = false, bool field = false, bool ignoreResistances = false,
	                     bool meleeHit = false) override;

	bool isInSpawnRange(const Position& pos) const;

	static bool pushItem(const Position& fromPos, Item* item);
	static void pushItems(const Position& fromPos, Tile* fromTile);
	static bool pushCreature(const Position& fromPos, Creature* creature);
	static bool pushCreatures(const Position& fromPos, Tile* fromTile, Creature* pushingCreature = nullptr);

	static uint32_t monsterAutoID;

private:
	std::string name;
	std::string nameDescription;

	MonsterType* mType;
	BaseSpawn* spawn = nullptr;
	BaseSpawn* originalSpawn = nullptr;

	STATE State = STATE::IDLE;

	Creature* Target = nullptr;

	uint64_t lifeTimeExpiration = 0;
	int64_t earliestMeleeAttack = 0;
	int32_t minCombatValue = 0;
	int32_t maxCombatValue = 0;
	uint32_t spawnInterval = 0;
	uint32_t currentSkill = 0;
	uint32_t skillCurrentExp = 0;
	uint32_t skillFactorPercent = 1000;
	uint32_t skillNextLevel = 0;
	uint32_t skillLearningPoints = 30;

	LightInfo internalLight{};

	Position masterPos;

	bool isIdle = true;
	bool pathBlockCheck = false;
	bool chaseMode = false;
	bool isRaidBoss = false;
	RaidPtr raidEvent = nullptr;

	std::array<Item*, CONST_SLOT_LAST + 1> inventory{};

	void addMonsterItemInventory(Container* bagItem, Item* item);

	void onCreatureEnter(Creature* creature);
	void onCreatureLeave(Creature* creature);
	void onCreatureFound(Creature* creature, bool pushFront = false);

	void updateLookDirection();

	void addSkillPoint();

	void death(Creature* lastHitCreature) override;
	Item* getCorpse(Creature* lastHitCreature, Creature* mostDamageCreature) override;

	void setIdle(bool idle);
	bool getIdleStatus() const { return isIdle; }

	void onAddCondition(ConditionType_t type) override;
	void onEndCondition(ConditionType_t type) override;

	void onAttackedCreature(Creature* creature, bool addInFightTicks = true) override;
	void onAttackedCreatureBlockHit(BlockType_t blockType, bool meleeHit = false) override;
	void onAttackedCreatureDrainHealth(Creature* target, int32_t points) override;

	bool canWalkTo(Position pos, Direction dir) const;

	uint64_t getLostExperience() const override { return skillLoss ? mType->info.experience : 0; }
	uint16_t getLookCorpse() const override { return mType->info.lookcorpse; }
	void dropLoot(Container* corpse, Creature* lastHitCreature) override;
	uint32_t getDamageImmunities() const override { return mType->info.damageImmunities; }
	uint32_t getConditionImmunities() const override { return mType->info.conditionImmunities; }

	friend class LuaScriptInterface;
	friend class Creature;
	friend class Game;
	friend class Tile;
	friend class MagicField;
	friend class Raids;
	friend class AreaSpawnEvent;
	friend class SingleSpawnEvent;
};
