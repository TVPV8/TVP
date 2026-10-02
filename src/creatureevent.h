// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include "luascript.h"
#include "enums.h"
#include "script.h"

class CreatureEvent;
using CreatureEvent_ptr = std::unique_ptr<CreatureEvent>;

enum CreatureEventType_t
{
	CREATURE_EVENT_NONE,
	CREATURE_EVENT_LOGIN,
	CREATURE_EVENT_LOGOUT,
	CREATURE_EVENT_LEAVEGAME,
	CREATURE_EVENT_THINK,
	CREATURE_EVENT_PREPAREDEATH,
	CREATURE_EVENT_DEATH,
	CREATURE_EVENT_KILL,
	CREATURE_EVENT_ADVANCE,
	CREATURE_EVENT_MODALWINDOW,
	CREATURE_EVENT_TEXTEDIT,
	CREATURE_EVENT_HEALTHCHANGE,
	CREATURE_EVENT_MANACHANGE,
	CREATURE_EVENT_EXTENDED_OPCODE, // otclient additional network opcodes
};

class CreatureEvent final : public ScriptEvent
{
public:
	explicit CreatureEvent(LuaScriptInterface* interface) :
	    ScriptEvent(interface), type(CREATURE_EVENT_NONE), loaded(false)
	{}

	CreatureEventType_t getEventType() const { return type; }
	void setEventType(CreatureEventType_t eventType) { type = eventType; }
	const std::string& getName() const { return eventName; }
	void setName(const std::string& name) { eventName = name; }
	bool isLoaded() const { return loaded; }
	void setLoaded(bool b) { loaded = b; }

	void clearEvent();
	void copyEvent(CreatureEvent* creatureEvent);

	// scripting
	bool executeOnLogin(Player* player) const;
	bool executeOnLogout(Player* player) const;
	bool executeOnLeaveGame(Player* player) const;
	bool executeOnThink(Creature* creature, uint32_t interval);
	bool executeOnPrepareDeath(Creature* creature, Creature* killer);
	bool executeOnDeath(Creature* creature, Item* corpse, Creature* killer, Creature* mostDamageKiller,
	                    bool lastHitUnjustified, bool mostDamageUnjustified);
	void executeOnKill(Creature* creature, Creature* target);
	bool executeAdvance(Player* player, skills_t, uint32_t, uint32_t);
	void executeModalWindow(Player* player, uint32_t modalWindowId, uint8_t buttonId, uint8_t choiceId);
	bool executeTextEdit(Player* player, Item* item, const std::string& text);
	void executeHealthChange(Creature* creature, Creature* attacker, CombatDamage& damage);
	void executeManaChange(Creature* creature, Creature* attacker, CombatDamage& damage);
	void executeExtendedOpcode(Player* player, uint8_t opcode, const std::string& buffer);
	//

private:
	std::string getScriptEventName() const override;

	std::string eventName;
	CreatureEventType_t type;
	bool loaded;
};

class CreatureEvents final
{
public:
	CreatureEvents();

	// non-copyable
	CreatureEvents(const CreatureEvents&) = delete;
	CreatureEvents& operator=(const CreatureEvents&) = delete;

	// global events
	bool playerLogin(Player* player) const;
	bool playerLogout(Player* player) const;
	bool playerLeaveGame(Player* player) const;
	bool playerAdvance(Player* player, skills_t, uint32_t, uint32_t);

	CreatureEvent* getEventByName(const std::string& name, bool forceLoaded = true);

	bool registerLuaEvent(CreatureEvent* event);
	void clear();

	void removeInvalidEvents();

private:
	LuaScriptInterface& getScriptInterface();
	std::string getScriptBaseName() const;

	// creature events
	using CreatureEventMap = std::map<std::string, CreatureEvent>;
	CreatureEventMap creatureEvents;

	LuaScriptInterface scriptInterface;
};
