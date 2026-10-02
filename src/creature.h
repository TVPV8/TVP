// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include "map.h"
#include "position.h"
#include "condition.h"
#include "const.h"
#include "tile.h"
#include "enums.h"
#include "creatureevent.h"

using ConditionList = std::list<Condition*>;
using CreatureEventList = std::list<CreatureEvent*>;

enum slots_t : uint8_t
{
	CONST_SLOT_WHEREEVER = 0,
	CONST_SLOT_HEAD = 1,
	CONST_SLOT_NECKLACE = 2,
	CONST_SLOT_BACKPACK = 3,
	CONST_SLOT_ARMOR = 4,
	CONST_SLOT_RIGHT = 5,
	CONST_SLOT_LEFT = 6,
	CONST_SLOT_LEGS = 7,
	CONST_SLOT_FEET = 8,
	CONST_SLOT_RING = 9,
	CONST_SLOT_AMMO = 10,

	CONST_SLOT_FIRST = CONST_SLOT_HEAD,
	CONST_SLOT_LAST = CONST_SLOT_AMMO,
};

struct FindPathParams
{
	bool fullPathSearch = true;
	bool clearSight = true;
	bool allowDiagonal = true;
	bool keepDistance = false;
	int32_t maxSearchDist = 0;
	int32_t minTargetDist = -1;
	int32_t maxTargetDist = -1;
};

enum ToDoType_t : uint8_t
{
	TODO_NONE,
	TODO_WAIT,
	TODO_ATTACK,
	TODO_ACTION,
	TODO_WALK,
	TODO_USEEX,
};

struct ToDoEntry
{
	ToDoType_t type = TODO_NONE;
	int64_t time = 0;
	std::function<void(void)> function;
};

class Map;
class Thing;
class Container;
class Player;
class Monster;
class Npc;
class Item;
class Tile;

static constexpr int32_t EVENT_CREATURECOUNT = 10;
static constexpr int32_t EVENT_CREATURE_THINK_INTERVAL = 1000;
static constexpr int32_t EVENT_CHECK_CREATURE_INTERVAL = (EVENT_CREATURE_THINK_INTERVAL / EVENT_CREATURECOUNT);
static constexpr int32_t CREATURE_DAMAGEMAP_SIZE = 20;

class FrozenPathingConditionCall
{
public:
	explicit FrozenPathingConditionCall(Position targetPos) : targetPos(std::move(targetPos)) {}

	bool operator()(const Position& startPos, const Position& testPos, const FindPathParams& fpp,
	                int32_t& bestMatchDist) const;

	bool isInRange(const Position& startPos, const Position& testPos, const FindPathParams& fpp) const;

private:
	Position targetPos;
};

//////////////////////////////////////////////////////////////////////
// Defines the Base class for all creatures and base functions which
// every creature has

class Creature : virtual public Thing
{
protected:
	Creature();

public:
	virtual ~Creature();

	// non-copyable
	Creature(const Creature&) = delete;
	Creature& operator=(const Creature&) = delete;

	Creature* getCreature() override final { return this; }
	const Creature* getCreature() const override final { return this; }
	virtual Player* getPlayer() { return nullptr; }
	virtual const Player* getPlayer() const { return nullptr; }
	virtual Npc* getNpc() { return nullptr; }
	virtual const Npc* getNpc() const { return nullptr; }
	virtual Monster* getMonster() { return nullptr; }
	virtual const Monster* getMonster() const { return nullptr; }

	virtual const std::string& getName() const = 0;
	virtual const std::string& getNameDescription() const = 0;

	virtual CreatureType_t getType() const = 0;

	virtual void setID() = 0;
	void setRemoved() { isInternalRemoved = true; }

	uint32_t getID() const { return id; }
	virtual void removeList() = 0;
	virtual void addList() = 0;

	virtual bool canSee(const Position& pos) const;
	virtual bool canSeeCreature(const Creature* creature) const;

	virtual RaceType_t getRace() const { return RACE_NONE; }
	virtual Skulls_t getSkull() const { return skull; }
	virtual Skulls_t getSkullClient(const Creature* creature) const { return creature->getSkull(); }
	void setSkull(Skulls_t newSkull);
	Direction getDirection() const { return direction; }
	void setDirection(Direction dir) { direction = dir; }

	bool isHealthHidden() const { return hiddenHealth; }
	void setHiddenHealth(bool b) { hiddenHealth = b; }

	int32_t getThrowRange() const override final { return 1; }
	bool isPushable() const override { return true; }
	bool isRemoved() const override final { return isInternalRemoved; }
	virtual bool canSeeInvisibility() const { return false; }
	virtual bool isInGhostMode() const { return false; }
	virtual bool canSeeGhostMode(const Creature*) const { return false; }

	virtual int32_t getStepSpeed() const { return getSpeed(); }
	int32_t getSpeed() const
	{
		if (baseSpeed == 0) return 0;
		return (2 * (baseSpeed + varSpeed)) + 80;
	}
	int32_t getVarSpeed() const { return varSpeed; }
	void setVarSpeed(int32_t varSpeedDelta) { varSpeed = varSpeedDelta; }

	void setBaseSpeed(uint32_t newBaseSpeed) { baseSpeed = newBaseSpeed; }
	uint32_t getBaseSpeed() const { return baseSpeed; }

	int32_t getHealth() const { return health; }
	virtual int32_t getMaxHealth() const { return healthMax; }

	const Outfit_t& getCurrentOutfit() const { return currentOutfit; }
	void setCurrentOutfit(Outfit_t outfit) { currentOutfit = outfit; }
	const Outfit_t& getDefaultOutfit() const { return defaultOutfit; }
	bool isInvisible() const;
	ZoneType_t getZone() const { return getTile()->getZone(); }

	// walk events
	virtual void onWalk(Direction& dir, uint32_t& flags);
	virtual void onWalkAborted() {}
	virtual void onWalkComplete() {}

	virtual bool getRandomStep(Direction& dir, bool checkForTiles = true) const;

	// follow functions
	Creature* getFollowCreature() const { return followCreature; }
	virtual bool setFollowCreature(Creature* creature);

	// follow events
	virtual void onFollowCreature(const Creature*) {}

	// combat functions
	Creature* getAttackedCreature() { return attackedCreature; }
	virtual bool setAttackedCreature(Creature* creature);
	virtual BlockType_t blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
	                             bool checkDefense = false, bool checkArmor = false, bool field = false,
	                             bool ignoreResistances = false, bool meleeHit = false);

	bool setMaster(Creature* newMaster);

	void removeMaster()
	{
		if (master) {
			master = nullptr;
			decrementReferenceCounter();
		}
	}

	bool isSummon() const { return master != nullptr; }
	Creature* getMaster() const { return master; }

	const std::list<Creature*>& getSummons() const { return summons; }

	virtual int32_t getArmor() const { return 0; }
	virtual int32_t getDefense() const { return 0; }
	virtual float getAttackFactor() const { return 1.0f; }
	virtual float getDefenseFactor() const { return 1.0f; }

	bool addCondition(Condition* condition);
	bool addCombatCondition(Condition* condition);
	void removeCondition(ConditionType_t type, ConditionId_t conditionId);
	void removeCondition(ConditionType_t type);
	void removeCondition(Condition* condition);
	void removeCombatCondition(ConditionType_t type);
	Condition* getCondition(ConditionType_t type) const;
	Condition* getCondition(ConditionType_t type, ConditionId_t conditionId, uint32_t subId = 0) const;
	void executeConditions(uint32_t interval);
	bool hasCondition(ConditionType_t type, uint32_t subId = 0) const;
	virtual bool isImmune(ConditionType_t type) const;
	virtual bool isImmune(CombatType_t type) const;
	virtual bool isSuppress(ConditionType_t type) const;
	virtual uint32_t getDamageImmunities() const { return 0; }
	virtual uint32_t getConditionImmunities() const { return 0; }
	virtual uint32_t getConditionSuppressions() const { return 0; }
	virtual bool isAttackable() const { return true; }

	virtual void changeHealth(int32_t healthChange, bool sendHealthChange = true);

	void gainHealth(Creature* healer, int32_t healthGain);
	virtual void drainHealth(Creature* attacker, int32_t damage);

	virtual bool challengeCreature(Creature*, bool) { return false; }

	CreatureVector getKillers() const;
	void onDeath();
	void distributeExperiencePoints();
	void addDamagePoints(const Creature* attacker, int32_t damagePoints);
	bool hasBeenAttacked(uint32_t attackerId);

	// combat event functions
	virtual void onAddCondition(ConditionType_t type);
	virtual void onAddCombatCondition(ConditionType_t type);
	virtual void onEndCondition(ConditionType_t type);
	void onTickCondition(ConditionType_t type, bool& bRemove);
	virtual void onCombatRemoveCondition(Condition* condition);
	virtual void onAttackedCreature(Creature*, bool = true) {}
	virtual void onAttacked();
	virtual void onAttackedCreatureDrainHealth(Creature* target, int32_t points);
	virtual void onTargetCreatureGainHealth(Creature*, int32_t) {}
	virtual bool onKilledCreature(Creature* target, bool lastHit = true);
	virtual void onGainExperience(uint64_t gainExp, Creature* target);
	virtual void onAttackedCreatureBlockHit(BlockType_t, bool meleeHit = false) {}
	virtual void onBlockHit() {}
	virtual void onChangeZone(ZoneType_t zone);
	virtual void onAttackedCreatureChangeZone(ZoneType_t zone);
	virtual void onIdleStatus();

	virtual LightInfo getCreatureLight() const;

	virtual void onIdleStimulus()
	{
		//
	}
	virtual void onThink(uint32_t interval);
	void onAttacking();

	virtual void onUpdateTileItem(const Tile*, const Position&, const Item*, const ItemType&, const Item*,
	                              const ItemType&)
	{}
	virtual void onRemoveTileItem(const Tile*, const Position&, const ItemType&, const Item*) {}

	virtual void onCreatureAppear(Creature* creature, bool isLogin);
	virtual void onRemoveCreature(Creature* creature, bool isLogout);
	virtual void onCreatureMove(Creature* creature, const Tile* newTile, const Position& newPos, const Tile* oldTile,
	                            const Position& oldPos, bool teleport);

	virtual void onAttackedCreatureDisappear(bool) {}
	virtual void onFollowCreatureDisappear(bool) {}

	virtual void onCreatureSay(Creature*, SpeakClasses, const std::string&) {}

	virtual void onPlacedCreature() {}

	virtual bool getCombatValues(int32_t&, int32_t&) { return false; }

	size_t getSummonCount() const { return summons.size(); }
	void setDropLoot(bool lootDrop) { this->lootDrop = lootDrop; }
	void setSkillLoss(bool skillLoss) { this->skillLoss = skillLoss; }
	void setUseDefense(bool useDefense) { canUseDefense = useDefense; }
	void setMovementBlocked(bool state) { movementBlocked = state; }
	bool isMovementBlocked() const { return movementBlocked; }

	// creature script events
	bool registerCreatureEvent(const std::string& name);
	bool unregisterCreatureEvent(const std::string& name);

	Cylinder* getParent() const override final { return currentTile; }
	void setParent(Cylinder* cylinder) override final
	{
		currentTile = static_cast<Tile*>(cylinder);
		position = currentTile->getPosition();
	}

	const Position& getPosition() const override final { return position; }

	Tile* getTile() override final { return currentTile; }
	const Tile* getTile() const override final { return currentTile; }

	const Position& getLastPosition() const { return lastPosition; }
	void setLastPosition(Position newLastPos) { lastPosition = newLastPos; }

	static bool canSee(const Position& myPos, const Position& pos, int32_t viewRangeX, int32_t viewRangeY);

	int32_t getDamageDealtByAttacker(const Creature* attacker) const;

	bool getPathTo(const Position& targetPos, std::vector<Direction>& dirList, const FindPathParams& fpp);
	bool getPathTo(const Position& targetPos, std::vector<Direction>& dirList, int32_t minTargetDist,
	               int32_t maxTargetDist, bool fullPathSearch = true, bool clearSight = true,
	               int32_t maxSearchDist = 0);

	int64_t calculateToDoDelay();
	void addYieldToDo();
	void addWaitToDo(int32_t delay);
	void addWalkToDo(const std::vector<Direction>& dirList, int32_t maxSteps = -1);
	void addWalkToDo(Direction dir);
	void addAttackToDo();
	void addActionToDo(ToDoType_t type, std::function<void(void)>&& function);
	void addActionToDo(std::function<void(void)>&& function);
	void stopToDo();
	bool clearToDo();
	void startToDo();
	void executeToDoEntries();

	void incrementReferenceCounter() { ++referenceCounter; }
	void decrementReferenceCounter()
	{
		if (--referenceCounter == 0) {
			delete this;
		}
	}

protected:
	struct CountBlock_t
	{
		uint32_t CreatureID = 0;
		int32_t total = 0;
		int64_t ticks = 0;
	};

	static constexpr int32_t mapWalkWidth = Map::maxViewportX * 2 + 1;
	static constexpr int32_t mapWalkHeight = Map::maxViewportY * 2 + 1;
	static constexpr int32_t maxWalkCacheWidth = (mapWalkWidth - 1) / 2;
	static constexpr int32_t maxWalkCacheHeight = (mapWalkHeight - 1) / 2;

	Position position;

	std::array<CountBlock_t, CREATURE_DAMAGEMAP_SIZE> damageMap;
	uint8_t actDamageEntry = 0;

	std::list<Creature*> summons;
	CreatureEventList eventsList;
	ConditionList conditions;

	bool isExecuting = false;
	bool stopExecuting = false;
	int32_t totalToDo = 0;
	int32_t currentToDo = 0;
	std::vector<ToDoEntry> toDoEntries;

	Tile* currentTile = nullptr;
	Creature* attackedCreature = nullptr;
	Creature* followCreature = nullptr;
	Creature* master = nullptr;

	uint64_t totalCombatDamageReceived = 0;
	uint64_t lastDefense = OTSYS_TIME();
	uint64_t earliestDefendTime = 0;
	int64_t earliestWalkTime = 0;
	int64_t earliestWakeUpTime = 0;

	uint32_t referenceCounter = 0;
	uint32_t id = 0;
	uint32_t scriptEventsBitField = 0;
	uint32_t lastHitCreatureId = 0;
	uint32_t blockCount = 0;
	uint32_t blockTicks = 0;
	uint32_t baseSpeed = 70;
	uint32_t lastKilledCreatureIdEvent = 0;
	int32_t varSpeed = 0;
	int32_t health = 1000;
	int32_t healthMax = 1000;

	Outfit_t currentOutfit;
	Outfit_t defaultOutfit;

	Position lastPosition;

	Direction direction = DIRECTION_SOUTH;
	Skulls_t skull = SKULL_NONE;

	bool isInternalRemoved = false;
	bool creatureCheck = false;
	bool inCheckCreaturesVector = false;
	bool skillLoss = true;
	bool lootDrop = true;
	bool hiddenHealth = false;
	bool canUseDefense = true;
	bool movementBlocked = false;

	// creature script events
	bool hasEventRegistered(CreatureEventType_t event) const
	{
		return (0 != (scriptEventsBitField & (static_cast<uint32_t>(1) << event)));
	}
	CreatureEventList getCreatureEvents(CreatureEventType_t type) const;

	void onCreatureDisappear(const Creature* creature, bool isLogout);
	virtual void doAttacking() {}

	virtual uint64_t getLostExperience() const { return 0; }
	virtual void dropLoot(Container*, Creature*) {}
	virtual uint16_t getLookCorpse() const { return 0; }
	virtual void death(Creature*) {}
	virtual bool dropCorpse(Creature* lastHitCreature, Creature* mostDamageCreature, bool lastHitUnjustified,
	                        bool mostDamageUnjustified);
	virtual Item* getCorpse(Creature* lastHitCreature, Creature* mostDamageCreature);

	friend class Game;
	friend class Map;
	friend class LuaScriptInterface;
	friend class Monster;
	friend class WeaponDistance;
};
