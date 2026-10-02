// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include "creature.h"
#include "luascript.h"

#include <set>

class Npc;
class Player;
class NpcBehavior;

class Npcs
{
public:
	static void reload();
};

class NpcScriptInterface final : public LuaScriptInterface
{
public:
	NpcScriptInterface();

	bool loadNpcLib(const std::string& file);

private:
	void registerFunctions();

	static int luaActionSay(lua_State* L);
	static int luaActionMove(lua_State* L);
	static int luaActionMoveTo(lua_State* L);
	static int luaActionTurn(lua_State* L);
	static int luaActionFollow(lua_State* L);
	static int luagetDistanceTo(lua_State* L);
	static int luaSetNpcFocus(lua_State* L);
	static int luaGetNpcCid(lua_State* L);
	static int luaGetNpcParameter(lua_State* L);
	static int luaDoSellItem(lua_State* L);

	// metatable
	static int luaNpcGetParameter(lua_State* L);
	static int luaNpcSetFocus(lua_State* L);

private:
	bool initState() override;
	bool closeState() override;

	bool libLoaded;
};

class NpcEventsHandler
{
public:
	NpcEventsHandler(const std::string& file, Npc* npc);

	void onCreatureAppear(Creature* creature);
	void onCreatureDisappear(Creature* creature);
	void onCreatureMove(Creature* creature, const Position& oldPos, const Position& newPos);
	void onCreatureSay(Creature* creature, SpeakClasses, const std::string& text);
	void onThink();

	bool isLoaded() const;

private:
	Npc* npc;
	NpcScriptInterface* scriptInterface;

	int32_t creatureAppearEvent = -1;
	int32_t creatureDisappearEvent = -1;
	int32_t creatureMoveEvent = -1;
	int32_t creatureSayEvent = -1;
	int32_t thinkEvent = -1;
	bool loaded = false;
};

class Npc final : public Creature
{
public:
	~Npc();

	// non-copyable
	Npc(const Npc&) = delete;
	Npc& operator=(const Npc&) = delete;

	Npc* getNpc() override { return this; }
	const Npc* getNpc() const override { return this; }

	bool isPushable() const override { return pushable && walkTicks != 0; }

	void setID() override
	{
		if (id == 0) {
			id = npcAutoID++;
		}
	}

	void removeList() override;
	void addList() override;

	static Npc* createNpc(const std::string& name);

	bool canSee(const Position& pos) const override;

	bool load();
	void reload();

	const std::string& getName() const override { return name; }
	const std::string& getNameDescription() const override { return name; }

	CreatureType_t getType() const override { return CREATURETYPE_NPC; }

	void doSay(const std::string& text);

	bool doMoveTo(const Position& pos, int32_t minTargetDist = 1, int32_t maxTargetDist = 1, bool fullPathSearch = true,
	              bool clearSight = true, int32_t maxSearchDist = 0);

	int32_t getMasterRadius() const { return masterRadius; }
	const Position& getMasterPos() const { return masterPos; }
	void setMasterPos(Position pos, int32_t radius = 1)
	{
		masterPos = pos;
		if (masterRadius == -1) {
			masterRadius = radius;
		}
	}

	void turnToCreature(Creature* creature);
	void setCreatureFocus(Creature* creature);

	NpcScriptInterface* getScriptInterface();

	static uint32_t npcAutoID;

private:
	explicit Npc(const std::string& name);

	void onCreatureAppear(Creature* creature, bool isLogin) override;
	void onRemoveCreature(Creature* creature, bool isLogout) override;
	void onCreatureMove(Creature* creature, const Tile* newTile, const Position& newPos, const Tile* oldTile,
	                    const Position& oldPos, bool teleport) override;

	void onCreatureSay(Creature* creature, SpeakClasses type, const std::string& text) override;
	void onIdleStimulus() override;
	void onThink(uint32_t interval) override;
	std::string getDescription(int32_t lookDistance) const override;

	bool isImmune(CombatType_t) const override { return !attackable; }
	bool isImmune(ConditionType_t type) const override
	{
		if (type == CONDITION_OUTFIT || type == CONDITION_DRUNK) {
			return false;
		}
		return !attackable;
	}
	bool isAttackable() const override { return attackable; }

	void setIdle(const bool idle);

	bool canWalkTo(const Position& fromPos, Direction dir) const;
	bool getRandomStep(Direction& dir, bool checkForTiles = true) const override;

	void reset();
	bool loadFromXml();

	std::map<std::string, std::string> parameters;

	std::set<Player*> spectators;

	std::string name;
	std::string filename;
	std::string behaviorFilename;

	NpcEventsHandler* npcEventHandler;

	Position masterPos;

	int64_t behaviorConversationTimeout = 0;
	uint32_t walkTicks;
	int32_t focusCreature;
	int32_t masterRadius;

	bool floorChange;
	bool attackable;
	bool ignoreHeight;
	bool loaded;
	bool isIdle;
	bool pushable;
	bool isBusy = false;
	bool reactionLock = false;

	NpcBehavior* npcBehavior = nullptr;

	static NpcScriptInterface* scriptInterface;

	friend class Npcs;
	friend class NpcScriptInterface;
	friend class NpcBehavior;
	friend class Game;
};
