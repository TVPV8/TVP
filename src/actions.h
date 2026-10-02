// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include "enums.h"
#include "luascript.h"
#include "script.h"

class Action;
using Action_ptr = std::unique_ptr<Action>;
using ActionFunction = std::function<bool(Player* player, Item* item, const Position& fromPosition, Thing* target,
                                          const Position& toPosition)>;

class Action : public ScriptEvent
{
public:
	explicit Action(LuaScriptInterface* interface);

	// scripting
	virtual bool executeUse(Player* player, Item* item, const Position& fromPosition, Thing* target,
	                        const Position& toPosition);

	bool getAllowFarUse() const { return allowFarUse; }
	void setAllowFarUse(bool v) { allowFarUse = v; }

	bool getCheckLineOfSight() const { return checkLineOfSight; }
	void setCheckLineOfSight(bool v) { checkLineOfSight = v; }

	bool getCheckFloor() const { return checkFloor; }
	void setCheckFloor(bool v) { checkFloor = v; }

	void clearItemIdRange() { return ids.clear(); }
	const std::vector<uint16_t>& getItemIdRange() const { return ids; }
	void addItemId(uint16_t id) { ids.emplace_back(id); }

	void clearUniqueIdRange() { return uids.clear(); }
	const std::vector<uint16_t>& getUniqueIdRange() const { return uids; }
	void addUniqueId(uint16_t id) { uids.emplace_back(id); }

	void clearActionIdRange() { return aids.clear(); }
	const std::vector<uint16_t>& getActionIdRange() const { return aids; }
	void addActionId(uint16_t id) { aids.emplace_back(id); }

	virtual ReturnValue canExecuteAction(const Player* player, const Position& toPos);
	virtual bool hasOwnErrorHandler() { return false; }
	virtual Thing* getTarget(Player* player, Creature* targetCreature, const Position& toPosition, uint8_t toStackPos,
	                         uint16_t spriteId) const;

private:
	std::string getScriptEventName() const override;

	bool allowFarUse = false;
	bool checkFloor = true;
	bool checkLineOfSight = true;
	std::vector<uint16_t> ids;
	std::vector<uint16_t> uids;
	std::vector<uint16_t> aids;
};

class Actions final
{
public:
	Actions();
	~Actions();

	// non-copyable
	Actions(const Actions&) = delete;
	Actions& operator=(const Actions&) = delete;

	bool useItem(Player* player, const Position& pos, uint8_t index, Item* item);
	bool useItemEx(Player* player, const Position& fromPos, const Position& toPos, uint8_t toStackPos,
	               uint16_t toSpriteId, Item* item, Creature* creature = nullptr);

	ReturnValue canUse(const Player* player, const Position& pos);
	ReturnValue canUse(const Player* player, const Position& pos, const Item* item);
	ReturnValue canUseFar(const Creature* creature, const Position& toPos, bool checkLineOfSight, bool checkFloor,
	                      bool isRune);

	bool registerLuaEvent(Action* event);
	void clear();

private:
	ReturnValue internalUseItem(Player* player, const Position& pos, uint8_t index, Item* item);

	LuaScriptInterface& getScriptInterface();
	std::string getScriptBaseName() const;

	using ActionUseMap = std::map<uint16_t, Action>;
	ActionUseMap useItemMap;
	ActionUseMap uniqueItemMap;
	ActionUseMap actionItemMap;

	Action* getAction(const Item* item);
	void clearMap(ActionUseMap& map);

	LuaScriptInterface scriptInterface;
};
