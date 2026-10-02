// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include "const.h"
#include "position.h"
#include "monsters.h"

struct MonsterSpawn
{
	MonsterSpawn(std::string name, uint32_t minAmount, uint32_t maxAmount, uint32_t spread, uint64_t lifetime) :
	    name(std::move(name)), minAmount(minAmount), maxAmount(maxAmount), spread(spread), lifetime(lifetime)
	{}

	std::vector<LootBlock> extraLoot;
	std::string name;
	uint32_t minAmount;
	uint32_t maxAmount;
	uint32_t spread;
	uint64_t lifetime;
};

// How many times it will try to find a tile to add the monster to before giving up
static constexpr int32_t MAXIMUM_TRIES_PER_MONSTER = 10;
static constexpr int32_t CHECK_RAIDS_INTERVAL = 10 * 1000;

class Raid;
class RaidEvent;

using RaidPtr = std::shared_ptr<Raid>;
using RaidEventPtr = std::shared_ptr<RaidEvent>;

class Raids
{
public:
	Raids();

	// non-copyable
	Raids(const Raids&) = delete;
	Raids& operator=(const Raids&) = delete;

	bool loadFromXml();
	bool startup();

	void clear();
	bool reload();

	bool isLoaded() const { return loaded; }
	bool isStarted() const { return started; }

	RaidPtr getRunning() { return running; }
	void setRunning(const RaidPtr& newRunning) { running = newRunning; }

	RaidPtr getRaidByName(const std::string& name) const;

	void checkRaids();

	LuaScriptInterface& getScriptInterface() { return scriptInterface; }

private:
	LuaScriptInterface scriptInterface{"Raid Interface"};

	std::list<RaidPtr> raidList;
	RaidPtr running = nullptr;
	uint32_t checkRaidsEvent = 0;
	bool loaded = false;
	bool started = false;
};

class Raid : public std::enable_shared_from_this<Raid>
{
public:
	Raid(std::string name, uint32_t interval) : name(std::move(name)), interval(interval) {}

	// non-copyable
	Raid(const Raid&) = delete;
	Raid& operator=(const Raid&) = delete;

	bool loadFromXml(const std::string& filename);

	void startRaid();

	void executeRaidEvent(const RaidEventPtr& raidEvent);
	void resetRaid();
	void reschedule();

	RaidEventPtr getNextRaidEvent() const;

	const std::string& getName() const { return name; }

	bool isLoaded() const { return loaded; }
	uint32_t getInterval() const { return interval; }

	const time_t& getDateTime() const { return datetime; }
	void setDateTime(time_t time) { datetime = time; }

	void setLogged(bool v) { log = v; }
	bool isLogged() const { return log; }

	void setExecuted() { executed = true; }
	bool hasExecuted() const { return executed; }

	void stopEvents();

private:
	std::vector<RaidEventPtr> raidEvents;
	std::string name;
	time_t datetime = 0;
	uint32_t minmargin = 0;
	uint32_t maxmargin = 0;
	uint32_t interval;
	uint32_t nextEvent = 0;
	uint32_t nextEventEvent = 0;
	uint32_t serverSaveMargin = 0;
	bool loaded = false;
	bool executed = false;
	bool log = false;
	bool repeatable = false;
	bool bossRaid = false;
	bool rescheduled = false;

	friend class Raids;
};

class RaidEvent
{
public:
	virtual ~RaidEvent() = default;

	virtual bool configureRaidEvent(const pugi::xml_node& eventNode);

	virtual bool executeEvent() = 0;
	uint32_t getDelay() const { return delay; }

protected:
	RaidPtr parentRaid = nullptr;
	uint32_t delay;

	friend class Raid;
};

class AnnounceEvent final : public RaidEvent
{
public:
	AnnounceEvent() = default;

	bool configureRaidEvent(const pugi::xml_node& eventNode) override;

	bool executeEvent() override;

private:
	std::string message;
	MessageClasses messageType = MESSAGE_EVENT_ADVANCE;
};

class SingleSpawnEvent final : public RaidEvent
{
public:
	bool configureRaidEvent(const pugi::xml_node& eventNode) override;

	bool executeEvent() override;

private:
	std::vector<LootBlock> extraLoot;
	std::string monsterName;
	Position position;
	bool bossSpawn = false;
};

class AreaSpawnEvent final : public RaidEvent
{
public:
	bool configureRaidEvent(const pugi::xml_node& eventNode) override;

	bool executeEvent() override;

private:
	std::list<MonsterSpawn> spawnList;
	Position fromPos, toPos;
	bool bossSpawn = false;
};

class RaidScriptEvent final : public RaidEvent, public ScriptEvent
{
public:
	explicit RaidScriptEvent(LuaScriptInterface* interface) : ScriptEvent(interface) {}

	bool configureRaidEvent(const pugi::xml_node& eventNode) override;

	bool executeEvent() override;

private:
	std::string getScriptEventName() const override;
};
