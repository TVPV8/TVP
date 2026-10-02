// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "spawn.h"
#include "game.h"
#include "monster.h"
#include "configmanager.h"
#include "scheduler.h"

#include "pugicast.h"
#include "events.h"

extern ConfigManager g_config;
extern Monsters g_monsters;
extern Game g_game;
extern Events* g_events;

static constexpr int32_t MINSPAWN_INTERVAL = 10 * 1000;           // 10 seconds to match RME
static constexpr int32_t MAXSPAWN_INTERVAL = 24 * 60 * 60 * 1000; // 1 day

int32_t Spawns::calculateSpawnDelay(int32_t delay)
{
	int32_t newDelay = delay;
	int32_t onlineCount = g_game.getPlayersOnline();
	if (onlineCount <= 800) {
		if (onlineCount > 200) {
			newDelay = 200 * newDelay / (onlineCount / 2 + 100);
		}
	} else {
		newDelay = 2 * newDelay / 5;
	}

	int32_t spawnRate = g_config.getNumber(ConfigManager::RATE_SPAWN);
	if (spawnRate != 0) {
		// minimum of 40 s respawn time
		newDelay = std::max<int32_t>(40000, newDelay / spawnRate);
	}

	return uniform_random(newDelay / 2, newDelay);
}

bool Spawns::loadFromXml(const std::string& filename)
{
	if (loaded) {
		return true;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.c_str());
	if (!result) {
		printXMLError("Error - Spawns::loadFromXml", filename, result);
		return false;
	}

	this->filename = filename;
	loaded = true;

	for (auto spawnNode : doc.child("spawns").children()) {
		Position centerPos(pugi::cast<uint16_t>(spawnNode.attribute("centerx").value()),
		                   pugi::cast<uint16_t>(spawnNode.attribute("centery").value()),
		                   pugi::cast<uint16_t>(spawnNode.attribute("centerz").value()));

		int32_t radius;
		pugi::xml_attribute radiusAttribute = spawnNode.attribute("radius");
		if (radiusAttribute) {
			radius = pugi::cast<int32_t>(radiusAttribute.value());
		} else {
			radius = -1;
		}

		Direction dir;

		pugi::xml_attribute directionAttribute = spawnNode.attribute("direction");
		if (directionAttribute) {
			dir = static_cast<Direction>(pugi::cast<uint16_t>(directionAttribute.value()));
		} else {
			dir = DIRECTION_NORTH;
		}

		pugi::xml_attribute amountAttribute = spawnNode.attribute("amount");
		if (amountAttribute) { // TVP Spawn system
			TvpSpawn* spawn = new TvpSpawn(centerPos, radius);
			tvpSpawnList.push_front(spawn);

			bool isNpc = false;
			std::string name;

			if (pugi::xml_attribute monsterName = spawnNode.attribute("monstername")) {
				name = monsterName.as_string();
			} else if (pugi::xml_attribute npcName = spawnNode.attribute("npcname")) {
				name = npcName.as_string();
				isNpc = true;
			}

			// tvp spawn system only supports one monsterhome per spawn
			if (!isNpc) {
				spawn->addMonster(name, centerPos, dir,
				                  static_cast<uint32_t>(spawnNode.attribute("spawntime").as_int() * 1000),
				                  static_cast<uint32_t>(spawnNode.attribute("amount").as_int()));
			} else {
				Npc* npc = Npc::createNpc(name);
				if (!npc) {
					continue;
				}

				if (directionAttribute) {
					npc->setDirection(static_cast<Direction>(pugi::cast<uint16_t>(directionAttribute.value())));
				}

				npc->setMasterPos(centerPos, radius);
				npcList.push_front(npc);
			}
		} else { // TFS Spawn system

			if (!spawnNode.first_child()) {
				std::cout << "[Warning - Spawns::loadFromXml] Empty spawn at position: " << centerPos
				          << " with radius: " << radius << '.' << std::endl;
				continue;
			}

			Spawn* spawn = new Spawn(centerPos, radius);
			spawnList.push_front(spawn);

			for (auto childNode : spawnNode.children()) {
				if (strcasecmp(childNode.name(), "monster") == 0) {
					pugi::xml_attribute nameAttribute = childNode.attribute("name");
					if (!nameAttribute) {
						continue;
					}

					Position pos(centerPos.x + pugi::cast<uint16_t>(childNode.attribute("x").value()),
					             centerPos.y + pugi::cast<uint16_t>(childNode.attribute("y").value()), centerPos.z);
					int32_t interval = pugi::cast<int32_t>(childNode.attribute("spawntime").value()) * 1000;
					if (interval >= MINSPAWN_INTERVAL && interval <= MAXSPAWN_INTERVAL) {
						spawn->addMonster(nameAttribute.as_string(), pos, dir, static_cast<uint32_t>(interval), 0);
					} else {
						if (interval < MINSPAWN_INTERVAL) {
							std::cout << "[Warning - Spawns::loadFromXml] " << nameAttribute.as_string() << ' ' << pos
							          << " spawntime can not be less than " << MINSPAWN_INTERVAL / 1000 << " seconds."
							          << std::endl;
						} else {
							std::cout << "[Warning - Spawns::loadFromXml] " << nameAttribute.as_string() << ' ' << pos
							          << " spawntime can not be more than " << MAXSPAWN_INTERVAL / 1000 << " seconds."
							          << std::endl;
						}
					}
				} else if (strcasecmp(childNode.name(), "npc") == 0) {
					pugi::xml_attribute nameAttribute = childNode.attribute("name");
					if (!nameAttribute) {
						continue;
					}

					Npc* npc = Npc::createNpc(nameAttribute.as_string());
					if (!npc) {
						continue;
					}

					if (directionAttribute) {
						npc->setDirection(static_cast<Direction>(pugi::cast<uint16_t>(directionAttribute.value())));
					}

					npc->setMasterPos(
					    Position(centerPos.x + pugi::cast<uint16_t>(childNode.attribute("x").value()),
					             centerPos.y + pugi::cast<uint16_t>(childNode.attribute("y").value()), centerPos.z),
					    radius);
					npcList.push_front(npc);
				}
			}
		}
	}

	return true;
}

void Spawns::startup()
{
	if (isStarted()) {
		return;
	}

	for (Npc* npc : npcList) {
		if (!g_game.placeCreature(npc, npc->getMasterPos(), true)) {
			std::cout << "[Warning - Spawns::startup] Couldn't spawn npc \"" << npc->getName()
			          << "\" on position: " << npc->getMasterPos() << '.' << std::endl;
			delete npc;
		}
	}
	npcList.clear();

	if (g_config.getBoolean(ConfigManager::DISABLE_MONSTER_SPAWNS)) {
		return;
	}

	for (BaseSpawn* spawn : spawnList) {
		spawn->startup();
	}

	for (BaseSpawn* spawn : tvpSpawnList) {
		spawn->startup();
	}

	started = true;
}

void Spawns::clear()
{
	for (BaseSpawn* spawn : spawnList) {
		spawn->stopSpawnCheck();
	}
	spawnList.clear();

	for (BaseSpawn* spawn : tvpSpawnList) {
		spawn->stopSpawnCheck();
	}
	tvpSpawnList.clear();

	started = false;
}

bool Spawns::isInZone(const Position& centerPos, int32_t radius, const Position& pos)
{
	if (radius == -1) {
		return true;
	}

	int32_t dx = std::abs(centerPos.getX() - pos.getX());
	int32_t dy = std::abs(centerPos.getY() - pos.getY());
	return dx <= radius && dy <= radius;
}

void Spawn::startup()
{
	for (auto& it : spawnMap) {
		spawnBlock_t& sb = it;
		spawnMonster(sb, true);
	}
}

void Spawn::addMonster(const std::string& name, const Position& pos, Direction& dir, uint32_t interval, uint32_t)
{
	spawnBlock_t sb;
	sb.mType = g_monsters.getMonsterType(name);
	if (!sb.mType) {
		std::cout << "Warning - [Spawn::addMonster] Could not find monster with name " << name << std::endl;
		return;
	}

	sb.direction = dir;
	sb.pos = pos;
	sb.interval = interval;
	spawnMap.push_back(sb);
}

void Spawn::checkSpawn()
{
	checkSpawnEvent = 0;

	if (activeMonsters >= spawnMap.size()) {
		// no need to respawn anymore monsters
		return;
	}

	for (auto& it : spawnMap) {
		spawnBlock_t& sb = it;
		if (OTSYS_TIME() >= sb.nextSpawnTime) {
			if (g_config.getBoolean(ConfigManager::SPAWNS_RESET_TIMER_ON_FAILED_SPAWN)) {
				sb.nextSpawnTime = OTSYS_TIME() + Spawns::calculateSpawnDelay(sb.interval);
			}

			if (!isPlayerAround(sb.pos)) {
				// we do not care if we successfully spawned the monster or not
				spawnMonster(sb);

				sb.nextSpawnTime = OTSYS_TIME() + Spawns::calculateSpawnDelay(sb.interval);

				if (g_config.getBoolean(ConfigManager::SPAWN_ONE_MONSTER_AT_A_TIME) ||
				    activeMonsters >= spawnMap.size()) {
					break;
				}
			}
		}
	}

	checkSpawnEvent =
	    g_scheduler.addEvent(createSchedulerTask(SPAWN_CHECK_INTERVAL, std::bind(&Spawn::checkSpawn, this)));
}

bool searchSpawnPosition(const Position& pos, Position& spawnPos)
{
	int32_t d = 0;
	int32_t c = 0;
	int32_t s = 1;
	uint16_t x = pos.x;
	uint16_t y = pos.y;
	int32_t totalNonSpawnableTiles = 0;

	for (int k = 1; k <= (64 - 1) && totalNonSpawnableTiles < 64; k++) {
		for (int j = 0; j < (k < (64 - 1) ? 2 : 3) && totalNonSpawnableTiles < 64; j++) {
			for (int i = 0; i < s && totalNonSpawnableTiles < 64; i++) {
				Tile* tile = g_game.map.getTile(x, y, pos.z);
				if (tile && tile->getGround()) {
					if (tile->getCreatureCount() > 0 || tile->hasFlag(TILESTATE_PROTECTIONZONE) ||
					    tile->hasFlag(TILESTATE_BLOCKSOLID) || tile->hasFlag(TILESTATE_BLOCKPATH)) {
						// cannot spawn in this tile
						c++;
						switch (d) {
							case 3:
								y = y + 1;
								break;
							case 0:
								x = x + 1;
								break;
							case 1:
								y = y - 1;
								break;
							case 2:
								x = x - 1;
								break;
						}

						if (tile->getCreatureCount() == 0 && !tile->hasFlag(TILESTATE_IMMOVABLEBLOCKSOLID) &&
						    !tile->hasFlag(TILESTATE_PROTECTIONZONE)) {
							spawnPos.x = x;
							spawnPos.y = y;
							spawnPos.z = pos.z;
						}
						totalNonSpawnableTiles++;
						continue;
					}

					spawnPos.x = x;
					spawnPos.y = y;
					spawnPos.z = pos.z;
					return true;
				}

				c++;
				switch (d) {
					case 3:
						y = y + 1;
						break;
					case 0:
						x = x + 1;
						break;
					case 1:
						y = y - 1;
						break;
					case 2:
						x = x - 1;
						break;
				}
			}

			d = (d + 1) % 4;
		}

		s = s + 1;
	}

	return false;
}

void TvpSpawn::startup()
{
	if (monsterSpawn.amount == 1) {
		spawnMonster(monsterSpawn.mType, monsterSpawn.pos, monsterSpawn.direction, monsterSpawn.interval, true);
	} else {
		for (uint32_t i = 1; i <= monsterSpawn.amount; i++) {
			Position pos = monsterSpawn.pos;
			if (searchSpawnPosition(pos, pos)) {
				spawnMonster(monsterSpawn.mType, pos, monsterSpawn.direction, monsterSpawn.interval, true);
			} else {
				Position urgentPos = pos;
				if (g_game.searchLoginField(nullptr, urgentPos.x, urgentPos.y, urgentPos.z, 1, true, false)) {
					spawnMonster(monsterSpawn.mType, urgentPos, monsterSpawn.direction, monsterSpawn.interval, true);
				}
			}
		}
	}
}

void TvpSpawn::addMonster(const std::string& name, const Position& pos, Direction& dir, uint32_t interval,
                          uint32_t amount)
{
	spawnBlock_t sb;
	sb.mType = g_monsters.getMonsterType(name);
	if (!sb.mType) {
		std::cout << "Warning - [Spawn::addMonster] Could not find monster with name " << name << std::endl;
		return;
	}

	sb.direction = dir;
	sb.pos = pos;
	sb.interval = interval;
	sb.amount = amount;
	monsterSpawn = sb;
}

void TvpSpawn::checkSpawn()
{
	checkSpawnEvent = 0;

	if (activeMonsters >= monsterSpawn.amount) {
		// no need to respawn anymore monsters
		return;
	}

	if (monsterSpawn.nextSpawnTime <= OTSYS_TIME()) {
		if (g_config.getBoolean(ConfigManager::SPAWNS_RESET_TIMER_ON_FAILED_SPAWN)) {
			monsterSpawn.nextSpawnTime = OTSYS_TIME() + Spawns::calculateSpawnDelay(monsterSpawn.interval);
		}

		if (g_config.getBoolean(ConfigManager::SPAWN_ALL_MONSTERS_PER_SPAWN)) {
			for (int32_t i = activeMonsters; i < monsterSpawn.amount; i++) {
				if (!isPlayerAround(monsterSpawn.pos)) {
					if (monsterSpawn.amount == 1) {
						spawnMonster(monsterSpawn.mType, monsterSpawn.pos, monsterSpawn.direction,
						             monsterSpawn.interval, true);
					} else {
						Position pos = monsterSpawn.pos;
						if (searchSpawnPosition(pos, pos)) {
							spawnMonster(monsterSpawn.mType, pos, monsterSpawn.direction, monsterSpawn.interval, true);
						} else {
							Position urgentPos = pos;
							if (g_game.searchLoginField(nullptr, urgentPos.x, urgentPos.y, urgentPos.z, 1, true,
							                            false)) {
								spawnMonster(monsterSpawn.mType, urgentPos, monsterSpawn.direction,
								             monsterSpawn.interval, true);
							}
						}
					}

					monsterSpawn.nextSpawnTime = OTSYS_TIME() + Spawns::calculateSpawnDelay(monsterSpawn.interval);
				}
			}
		} else {
			if (!isPlayerAround(monsterSpawn.pos)) {
				if (monsterSpawn.amount == 1) {
					spawnMonster(monsterSpawn.mType, monsterSpawn.pos, monsterSpawn.direction, monsterSpawn.interval,
					             true);
				} else {
					Position pos = monsterSpawn.pos;
					if (searchSpawnPosition(pos, pos)) {
						spawnMonster(monsterSpawn.mType, pos, monsterSpawn.direction, monsterSpawn.interval, true);
					} else {
						Position urgentPos = pos;
						if (g_game.searchLoginField(nullptr, urgentPos.x, urgentPos.y, urgentPos.z, 1, true, false)) {
							spawnMonster(monsterSpawn.mType, urgentPos, monsterSpawn.direction, monsterSpawn.interval,
							             true);
						}
					}
				}

				monsterSpawn.nextSpawnTime = OTSYS_TIME() + Spawns::calculateSpawnDelay(monsterSpawn.interval);
			}
		}
	}

	checkSpawnEvent =
	    g_scheduler.addEvent(createSchedulerTask(SPAWN_CHECK_INTERVAL, std::bind(&TvpSpawn::checkSpawn, this)));
}

void BaseSpawn::startSpawnCheck(uint32_t interval)
{
	if (checkSpawnEvent == 0) {
		checkSpawnEvent = g_scheduler.addEvent(
		    createSchedulerTask(Spawns::calculateSpawnDelay(interval), std::bind(&BaseSpawn::checkSpawn, this)));
	}
}

void BaseSpawn::stopSpawnCheck()
{
	if (checkSpawnEvent != 0) {
		g_scheduler.stopEvent(checkSpawnEvent);
		checkSpawnEvent = 0;
	}
}

bool BaseSpawn::isPlayerAround(const Position& pos)
{
	SpectatorVec spectators;
	g_game.map.getSpectators(spectators, pos, true, true, Map::maxClientViewportX, Map::maxClientViewportX,
	                         Map::maxClientViewportY, Map::maxClientViewportY);
	for (Creature* spectator : spectators) {
		if (spectator->canSee(pos) && !spectator->getPlayer()->hasFlag(PlayerFlag_IgnoredByMonsters)) {
			return true;
		}

		if (g_config.getBoolean(ConfigManager::SPAWN_MULTIFLOOR_RESPAWN_BLOCK)) {
			// If the client can see the area, it will not allow monsters from spawning.
			if (spectator->canSee(pos)) return true;
		}
	}

	return false;
}

bool BaseSpawn::spawnMonster(spawnBlock_t& sb, bool startup)
{
	bool isBlocked = !startup && isPlayerAround(sb.pos);
	if (isBlocked && !sb.mType->info.isIgnoringSpawnBlock) {
		return false;
	}

	return spawnMonster(sb.mType, sb.pos, sb.direction, sb.interval, startup);
}

bool BaseSpawn::spawnMonster(MonsterType* mType, const Position& pos, Direction dir, uint32_t interval, bool forceSpawn)
{
	std::unique_ptr<Monster> monster_ptr(new Monster(mType));
	if (!g_events->eventMonsterOnSpawn(monster_ptr.get(), pos, forceSpawn, false)) {
		return false;
	}

	if (forceSpawn) {
		if (g_game.getGameState() <= GAME_STATE_CLOSED) {
			if (!g_game.placeCreature(monster_ptr.get(), pos, true)) {
				std::cout << "[Warning - BaseSpawn::spawnMonster] Couldn't spawn monster \"" << monster_ptr->getName()
				          << "\" on position: " << pos << '.' << std::endl;
				return false;
			}
		} else {
			if (!g_game.internalPlaceCreature(monster_ptr.get(), pos, true)) {
				std::cout << "[Warning - BaseSpawn::spawnMonster] Couldn't spawn monster \"" << monster_ptr->getName()
				          << "\" on position: " << pos << '.' << std::endl;
				return false;
			}
		}
	} else {
		if (!g_game.placeCreature(monster_ptr.get(), pos, forceSpawn)) {
			return false;
		}
	}

	Monster* monster = monster_ptr.release();
	monster->setDirection(dir);
	monster->setSpawn(this);
	monster->setMasterPos(pos);
	monster->setSpawnInterval(interval);
	increaseMonsterCount();
	return true;
}
