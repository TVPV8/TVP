// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "pugicast.h"

#include "actions.h"
#include "bed.h"
#include "configmanager.h"
#include "creature.h"
#include "creatureevent.h"
#include "databasetasks.h"
#include "events.h"
#include "game.h"
#include "globalevent.h"
#include "iologindata.h"
#include "items.h"
#include "monster.h"
#include "movement.h"
#include "scheduler.h"
#include "server.h"
#include "spells.h"
#include "talkaction.h"
#include "weapons.h"
#include "script.h"

#include <fmt/format.h>

extern ConfigManager g_config;
extern Actions* g_actions;
extern Chat* g_chat;
extern TalkActions* g_talkActions;
extern Spells* g_spells;
extern Vocations g_vocations;
extern GlobalEvents* g_globalEvents;
extern CreatureEvents* g_creatureEvents;
extern Events* g_events;
extern Monsters g_monsters;
extern MoveEvents* g_moveEvents;
extern Weapons* g_weapons;
extern Scripts* g_scripts;

Game::~Game()
{
	for (const auto& it : guilds) {
		delete it.second;
	}
}

void Game::start(ServiceManager* manager)
{
	bootTime = std::time(nullptr);

	serviceManager = manager;
	updateWorldTime();

	if (g_config.getBoolean(ConfigManager::DEFAULT_WORLD_LIGHT)) {
		g_scheduler.addEvent(createSchedulerTask(EVENT_LIGHTINTERVAL, std::bind(&Game::checkLight, this)));
	}
	g_scheduler.addEvent(createSchedulerTask(EVENT_CREATURE_THINK_INTERVAL, std::bind(&Game::checkCreatures, this, 0)));
	g_scheduler.addEvent(createSchedulerTask(EVENT_DECAYINTERVAL, std::bind(&Game::checkDecay, this)));
}

bool Game::loadMainMap(const std::string& filename)
{
	return map.loadMap("data/world/" + filename + ".otbm", true);
}

void Game::loadMap(const std::string& path)
{
	map.loadMapPart(path, true, true);
}

GameState_t Game::getGameState() const
{
	return gameState;
}

void Game::setWorldType(WorldType_t type)
{
	worldType = type;
}

void Game::setGameState(GameState_t newState)
{
	if (gameState == GAME_STATE_SHUTDOWN) {
		return; //this cannot be stopped
	}

	if (gameState == newState) {
		return;
	}

	gameState = newState;
	switch (newState) {
		case GAME_STATE_INIT: {
			groups.load();
			g_chat->load();

			map.spawns.startup();

			std::cout << "> Total Monsters: " << g_game.getMonstersOnline() << std::endl;
			std::cout << "> Total NPCs: " << g_game.getNpcsOnline() << std::endl;

			raids.loadFromXml();
			raids.startup();

			loadMotdNum();
			loadPlayersRecord();
			loadAccountStorageValues();

			g_globalEvents->startup();

			if (sendPlayersToTemple && g_config.getBoolean(ConfigManager::UPON_MAP_UPDATE_SENDPLAYERS_TO_TEMPLE)) {
				std::cout << "> (Map was updated) - All players have been sent to their temple." << std::endl;

				Database::getInstance().executeQuery("UPDATE `players` SET `posx` = 0 WHERE 1;");
			}

			processCommunication();
			processRemovedCreatures();
			proceduralRefreshMap();
			processConditions();
			break;
		}

		case GAME_STATE_SHUTDOWN: {
			g_scheduler.stopEvent(eventRefreshId);

			g_globalEvents->execute(GLOBALEVENT_SHUTDOWN);

			//kick all players that are still online
			auto it = players.begin();
			while (it != players.end()) {
				it->second->kickPlayer(true, true);
				it = players.begin();
			}

			if (allowMapSave) {
				map.refreshMap();
			}

			saveMotdNum();
			saveGameState();

			g_dispatcher.addTask(
				createTask(std::bind(&Game::shutdown, this)));

			g_scheduler.stop();
			g_databaseTasks.stop();
			g_dispatcher.stop();
			break;
		}

		default:
			break;
	}
}

#if !defined(_MSC_VER)
__attribute__((used))
#endif
void Game::saveGameState()
{
	if (gameState == GAME_STATE_NORMAL) {
		setGameState(GAME_STATE_MAINTAIN);
	}

	std::cout << "> Saving game..." << std::endl;

	if (!saveAccountStorageValues()) {
		std::cout << "[Error - Game::saveGameState] Failed to save account-level storage values." << std::endl;
	}

	for (const auto& it : players) {
		IOLoginData::savePlayer(it.second);
	}

	Map::save();

	g_databaseTasks.flush();

	if (gameState == GAME_STATE_MAINTAIN) {
		setGameState(GAME_STATE_NORMAL);
	}
}

Cylinder* Game::internalGetCylinder(Player* player, const Position& pos) const
{
	if (pos.x != 0xFFFF) {
		return map.getTile(pos);
	}

	//container
	if (pos.y & 0x40) {
		uint8_t from_cid = pos.y & 0x0F;
		return player->getContainerByID(from_cid);
	}

	//inventory
	return player;
}

Thing* Game::internalGetThing(Player* player, const Position& pos, int32_t index, uint32_t /*spriteId*/, stackPosType_t type) const
{
	if (pos.x != 0xFFFF) {
		Tile* tile = map.getTile(pos);
		if (!tile) {
			return nullptr;
		}

		Thing* thing;
		switch (type) {
			case STACKPOS_LOOK: {
				if (House* house = tile->getHouse()) {
					if (Creature* creature = tile->getBottomVisibleCreature(player)) {
						return creature;
					}

					if (g_config.getBoolean(ConfigManager::HOUSE_DOORS_DISPLAY_HOUSEINFO)) {
						if (Door* door = tile->getDoorItem()) {
							return door;
						}
					}
				}

				if (thing = tile->getBottomVisibleCreature(player)) {
					return thing;
				}

				if (g_config.getBoolean(ConfigManager::CLASSIC_MONSTER_INVISIBILITY)) {
					if (thing = tile->getTopCreature()) {
						if (thing->getCreature()->isInGhostMode() && !player->canSeeGhostMode(thing->getCreature())) {
							thing = nullptr;
						}

						if (thing) {
							return thing;
						}
					}
				}

				return tile->getTopVisibleThing(player);
			}

			case STACKPOS_MOVE: {
				thing = tile->getThing(index);
				if (thing) {
					Item* item = thing->getItem();
					if (item && item->isMoveable()) {
						thing = item;
					} else {
						thing = tile->getBottomVisibleCreature(player);
						if (!thing) { // invisible creature in the tile, stackorder is different
							if (tile->getTopCreature()) {
								thing = tile->getThing(index);
							}
						}
					}
				}
				break;
			}

			case STACKPOS_USEITEM: {
				thing = tile->getUseItem();
				break;
			}

			case STACKPOS_TOPDOWN_ITEM: {
				thing = tile->getTopDownItem();
				break;
			}

			case STACKPOS_USETARGET: {
				thing = tile->getTopVisibleCreature(player);
				if (!thing) {
					thing = tile->getTopDownItem();
					if (!thing) {
						thing = tile->getTopTopItem();
						if (!thing) {
							thing = tile->getGround();
						}
					}
				}
				break;
			}

			default: {
				thing = nullptr;
				break;
			}
		}

		if (player && tile->hasFlag(TILESTATE_SUPPORTS_HANGABLE) && type != STACKPOS_USETARGET) {
			//do extra checks here if the thing is accessible
			if (thing && thing->getItem()) {
				if (tile->hasProperty(CONST_PROP_ISVERTICAL)) {
					if (player->getPosition().x + 1 == tile->getPosition().x) {
						thing = nullptr;
					}
				} else { // horizontal
					if (player->getPosition().y + 1 == tile->getPosition().y) {
						thing = nullptr;
					}
				}
			}
		}
		return thing;
	}

	//container
	if (pos.y & 0x40) {
		uint8_t fromCid = pos.y & 0x0F;

		Container* parentContainer = player->getContainerByID(fromCid);
		if (!parentContainer) {
			return nullptr;
		}

		uint8_t slot = pos.z;
		return parentContainer->getItemByIndex(player->getContainerIndex(fromCid) + slot);
	}

	//inventory
	slots_t slot = static_cast<slots_t>(pos.y);
	return player->getInventoryItem(slot);
}

void Game::internalGetPosition(Item* item, Position& pos, uint8_t& stackpos)
{
	pos.x = 0;
	pos.y = 0;
	pos.z = 0;
	stackpos = 0;

	Cylinder* topParent = item->getTopParent();
	if (topParent) {
		if (Player* player = dynamic_cast<Player*>(topParent)) {
			pos.x = 0xFFFF;

			Container* container = dynamic_cast<Container*>(item->getParent());
			if (container) {
				pos.y = static_cast<uint16_t>(0x40) | static_cast<uint16_t>(player->getContainerID(container));
				pos.z = container->getThingIndex(item);
				stackpos = pos.z;
			} else {
				pos.y = player->getThingIndex(item);
				stackpos = pos.y;
			}
		} else if (Tile* tile = topParent->getTile()) {
			pos = tile->getPosition();
			stackpos = tile->getThingIndex(item);
		}
	}
}

Creature* Game::getCreatureByID(uint32_t id)
{
	if (id <= Player::playerAutoID) {
		return getPlayerByID(id);
	} else if (id <= Monster::monsterAutoID) {
		return getMonsterByID(id);
	} else if (id <= Npc::npcAutoID) {
		return getNpcByID(id);
	}
	return nullptr;
}

Monster* Game::getMonsterByID(uint32_t id)
{
	if (id == 0) {
		return nullptr;
	}

	auto it = monsters.find(id);
	if (it == monsters.end()) {
		return nullptr;
	}
	return it->second;
}

Npc* Game::getNpcByID(uint32_t id)
{
	if (id == 0) {
		return nullptr;
	}

	auto it = npcs.find(id);
	if (it == npcs.end()) {
		return nullptr;
	}
	return it->second;
}

Player* Game::getPlayerByID(uint32_t id)
{
	if (id == 0) {
		return nullptr;
	}

	auto it = players.find(id);
	if (it == players.end()) {
		return nullptr;
	}
	return it->second;
}

Creature* Game::getCreatureByName(const std::string& s)
{
	if (s.empty()) {
		return nullptr;
	}

	const std::string& lowerCaseName = asLowerCaseString(s);

	{
		auto it = mappedPlayerNames.find(lowerCaseName);
		if (it != mappedPlayerNames.end()) {
			return it->second;
		}
	}

	auto equalCreatureName = [&](const std::pair<uint32_t, Creature*>& it) {
		auto name = it.second->getName();
		return lowerCaseName.size() == name.size() && std::equal(lowerCaseName.begin(), lowerCaseName.end(), name.begin(), [](char a, char b) {
			return a == std::tolower(b);
		});
	};

	{
		auto it = std::find_if(npcs.begin(), npcs.end(), equalCreatureName);
		if (it != npcs.end()) {
			return it->second;
		}
	}

	{
		auto it = std::find_if(monsters.begin(), monsters.end(), equalCreatureName);
		if (it != monsters.end()) {
			return it->second;
		}
	}

	return nullptr;
}

Npc* Game::getNpcByName(const std::string& s)
{
	if (s.empty()) {
		return nullptr;
	}

	const char* npcName = s.c_str();
	for (const auto& it : npcs) {
		if (strcasecmp(npcName, it.second->getName().c_str()) == 0) {
			return it.second;
		}
	}
	return nullptr;
}

Player* Game::getPlayerByName(const std::string& s)
{
	if (s.empty()) {
		return nullptr;
	}

	auto it = mappedPlayerNames.find(asLowerCaseString(s));
	if (it == mappedPlayerNames.end()) {
		return nullptr;
	}
	return it->second;
}

Player* Game::getPlayerByGUID(const uint32_t& guid)
{
	if (guid == 0) {
		return nullptr;
	}

	auto it = mappedPlayerGuids.find(guid);
	if (it == mappedPlayerGuids.end()) {
		return nullptr;
	}
	return it->second;
}

ReturnValue Game::getPlayerByNameWildcard(const std::string& s, Player*& player)
{
	size_t strlen = s.length();
	if (strlen == 0 || strlen >= PLAYER_NAME_MAXLENGTH) {
		return RETURNVALUE_PLAYERWITHTHISNAMEISNOTONLINE;
	}

	if (s.back() == '~') {
		const std::string& query = asLowerCaseString(s.substr(0, strlen - 1));
		std::string result;
		ReturnValue ret = wildcardTree.findOne(query, result);
		if (ret != RETURNVALUE_NOERROR) {
			return ret;
		}

		player = getPlayerByName(result);
	} else {
		player = getPlayerByName(s);
	}

	if (!player) {
		return RETURNVALUE_PLAYERWITHTHISNAMEISNOTONLINE;
	}

	return RETURNVALUE_NOERROR;
}

Player* Game::getPlayerByAccount(uint32_t acc)
{
	for (const auto& it : players) {
		if (it.second->getAccount() == acc) {
			return it.second;
		}
	}
	return nullptr;
}

bool Game::internalPlaceCreature(Creature* creature, const Position& pos, bool forced /*= false*/)
{
	if (creature->getParent() != nullptr) {
		return false;
	}

	if (!map.placeCreature(pos, creature, forced)) {
		return false;
	}

	creature->incrementReferenceCounter();
	creature->setID();
	creature->addList();
	return true;
}

bool Game::placeCreature(Creature* creature, const Position& pos, bool forced /*= false*/)
{
	if (!internalPlaceCreature(creature, pos, forced)) {
		return false;
	}

	SpectatorVec spectators;
	map.getSpectators(spectators, creature->getPosition(), true);
	for (Creature* spectator : spectators) {
		if (Player* tmpPlayer = spectator->getPlayer()) {
			tmpPlayer->sendCreatureAppear(creature, creature->getPosition());
		}
	}

	for (Creature* spectator : spectators) {
		spectator->onCreatureAppear(creature, true);
	}

	creature->getParent()->postAddNotification(creature, nullptr, 0);

	addCreatureCheck(creature);
	creature->onPlacedCreature();

	// Teleport effect only appears when a player spawns
	if (creature->getPlayer()) {
		addMagicEffect(creature->getPosition(), CONST_ME_TELEPORT);
	}

	return true;
}

bool Game::removeCreature(Creature* creature, bool isLogout/* = true*/)
{
	if (creature->isRemoved()) {
		return false;
	}

	Tile* tile = creature->getTile();

	std::vector<int32_t> oldStackPosVector;

	SpectatorVec spectators;
	map.getSpectators(spectators, tile->getPosition(), true);
	for (Creature* spectator : spectators) {
		if (Player* player = spectator->getPlayer()) {
			oldStackPosVector.push_back(player->canSeeCreature(creature) ? tile->getClientIndexOfCreature(player, creature) : -1);
		}
	}

	tile->removeCreature(creature);

	const Position& tilePosition = tile->getPosition();

	//send to client
	size_t i = 0;
	for (Creature* spectator : spectators) {
		if (Player* player = spectator->getPlayer()) {
			player->sendRemoveTileCreature(creature, tilePosition, oldStackPosVector[i++]);
		}
	}

	//event method
	for (Creature* spectator : spectators) {
		spectator->onRemoveCreature(creature, isLogout);
	}

	Creature* master = creature->getMaster();
	if (master && !master->isRemoved()) {
		creature->setMaster(nullptr);
	}

	creature->getParent()->postRemoveNotification(creature, nullptr, 0);

	creature->removeList();
	creature->setRemoved();
	ReleaseCreature(creature);

	removeCreatureCheck(creature);
	return true;
}

void Game::executeRemoveCreature(Creature* creature)
{
	if (creature && !creature->isRemoved()) {
		creature->incrementReferenceCounter();
		removedCreatures.insert(creature);
	}
}

void Game::executeDeath(Creature* creature)
{
	if (creature && !creature->isRemoved()) {
		creature->incrementReferenceCounter();
		killedCreatures.insert(creature);
	}
}

void Game::playerMoveThing(uint32_t playerId, const Position fromPos,
                           uint16_t spriteId, uint8_t fromStackPos, const Position toPos, uint8_t count)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	uint8_t fromIndex = 0;
	if (fromPos.x == 0xFFFF) {
		if (fromPos.y & 0x40) {
			fromIndex = fromPos.z;
		} else {
			fromIndex = static_cast<uint8_t>(fromPos.y);
		}
	} else {
		fromIndex = fromStackPos;
	}

	Thing* thing = internalGetThing(player, fromPos, fromIndex, spriteId, STACKPOS_MOVE);
	if (!thing) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		return;
	}

	/*
	 * This situation may occur upon Issue #187
	 * Since 7.4 Tibia supports moving items on a tile from any index (stackpos)
	 * When a creature steps on the tile the player is about to move an item
	 * That monster becomes the "index" of the item we're gonna move
	 * So we end up moving the monster instead of the item we expected to move
	 */
	if (thing->getCreature() && spriteId > 99) {
		const ItemType& iType = Item::items.getItemIdByClientId(spriteId);
		if (Item* clientItem = findItemOfType(map.getTile(fromPos), iType.id)) {
			thing = clientItem;
		}
	}

	if (Creature* movingCreature = thing->getCreature()) {
		Tile* tile = map.getTile(toPos);
		if (!tile) {
			player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
			return;
		}

		if (!Position::areInRange<1, 1, 0>(movingCreature->getPosition(), player->getPosition())) {
			//need to walk to the creature first before moving it
			std::vector<Direction> listDir;
			if (player->getPathTo(movingCreature->getPosition(), listDir, 0, 1, true, true)) {
				player->addWalkToDo(listDir);
			} else {
				player->sendCancelMessage(RETURNVALUE_THEREISNOWAY);
				return;
			}
		}

		if (movingCreature->getMonster()) {
			if (movingCreature->earliestWalkTime <= OTSYS_TIME()) {
				player->addWaitToDo(1000);
			} else {
				player->addWaitToDo(movingCreature->earliestWalkTime - OTSYS_TIME() + 1000);
			}
		} else {
			player->addWaitToDo(1000);
		}

		player->addActionToDo(std::bind(&Game::playerMoveCreatureByID, &g_game, player->getID(), movingCreature->getID(), movingCreature->getPosition(), tile->getPosition()));
		player->startToDo();
	} else if (Item* movingItem = thing->getItem()) {
		Cylinder* toCylinder = internalGetCylinder(player, toPos);
		if (!toCylinder) {
			player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
			return;
		}

		// This is strict 7.4 behavior
		if (movingItem->getID() != spriteId) {
			Tile* fromTile = map.getTile(fromPos);
			if (fromTile) {
				const ItemType& iType = Item::items.getItemIdByClientId(spriteId);
				Item* clientItem = findItemOfType(fromTile, iType.id);
				if (clientItem) {
					thing = clientItem; // override server found item
				}
			}
		}

		playerMoveItem(player, fromPos, spriteId, fromStackPos, toPos, count, thing->getItem(), toCylinder);
	}

	player->resetIdleTime();
}

void Game::playerMoveCreatureByID(uint32_t playerId, uint32_t movingCreatureId, const Position movingCreatureOrigPos, const Position toPos)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Creature* movingCreature = getCreatureByID(movingCreatureId);
	if (!movingCreature) {
		return;
	}

	Tile* toTile = map.getTile(toPos);
	if (!toTile) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		return;
	}

	if (!Position::areInRange<1, 1, 0>(player->getPosition(), movingCreature->getPosition())) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		return;
	}

	playerMoveCreature(player, movingCreature, movingCreatureOrigPos, toTile);
}

void Game::playerMoveCreature(Player* player, Creature* movingCreature, const Position movingCreatureOrigPos, Tile* toTile)
{
	if (movingCreature->isMovementBlocked()) {
		player->sendCancelMessage(RETURNVALUE_NOTMOVEABLE);
		return;
	}

	if (!Position::areInRange<1, 1, 0>(movingCreatureOrigPos, player->getPosition())) {
		//need to walk to the creature first before moving it
		std::vector<Direction> listDir;
		if (player->getPathTo(movingCreatureOrigPos, listDir, 0, 1, true, true)) {
			player->addWalkToDo(listDir);
			player->addActionToDo(std::bind(&Game::playerMoveCreatureByID, this,
				player->getID(), movingCreature->getID(), movingCreatureOrigPos, toTile->getPosition()));
			player->startToDo();
		} else {
			player->sendCancelMessage(RETURNVALUE_THEREISNOWAY);
		}
		return;
	}

	if ((!movingCreature->isPushable() && !player->hasFlag(PlayerFlag_CanPushAllCreatures)) ||
	        (movingCreature->isInGhostMode() && !player->canSeeGhostMode(movingCreature))) {
		player->sendCancelMessage(RETURNVALUE_NOTMOVEABLE);
		return;
	}

	//check throw distance
	const Position& movingCreaturePos = movingCreature->getPosition();
	const Position& toPos = toTile->getPosition();
	if ((Position::getDistanceX(movingCreaturePos, toPos) > movingCreature->getThrowRange()) || (Position::getDistanceY(movingCreaturePos, toPos) > movingCreature->getThrowRange()) || (Position::getDistanceZ(movingCreaturePos, toPos) * 4 > movingCreature->getThrowRange())) {
		player->sendCancelMessage(RETURNVALUE_DESTINATIONOUTOFREACH);
		return;
	}

	if (player != movingCreature) {
		if (toTile->hasFlag(TILESTATE_BLOCKPATH)) {
			player->sendCancelMessage(RETURNVALUE_NOTENOUGHROOM);
			return;
		} else if ((movingCreature->getZone() == ZONE_PROTECTION && !toTile->hasFlag(TILESTATE_PROTECTIONZONE)) || (movingCreature->getZone() == ZONE_NOPVP && !toTile->hasFlag(TILESTATE_NOPVPZONE))) {
			player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
			return;
		} else {
			if (CreatureVector* tileCreatures = toTile->getCreatures()) {
				for (Creature* tileCreature : *tileCreatures) {
					if (!tileCreature->isInGhostMode()) {
						player->sendCancelMessage(RETURNVALUE_NOTENOUGHROOM);
						return;
					}
				}
			}

			Npc* movingNpc = movingCreature->getNpc();
			if (movingNpc && !Spawns::isInZone(movingNpc->getMasterPos(), movingNpc->getMasterRadius(), toPos)) {
				player->sendCancelMessage(RETURNVALUE_NOTENOUGHROOM);
				return;
			}
		}
	}

	if (!g_events->eventPlayerOnMoveCreature(player, movingCreature, movingCreaturePos, toPos)) {
		return;
	}

	ReturnValue ret = internalMoveCreature(*movingCreature, *toTile);
	if (ret != RETURNVALUE_NOERROR) {
		player->sendCancelMessage(ret);
	}

	// Pushing creatures delays the attack by an extra 2s
	player->earliestAttackTime = OTSYS_TIME() + player->getAttackSpeed();
}

void Game::moveCreature(Creature* creature, Direction direction, uint32_t flags) 
{
	if (creature->getSpeed() == 0 || creature->isRemoved() || creature->getHealth() <= 0) {
		return;
	}

	const Position toPos = getNextPosition(direction, creature->getPosition());
	if (Monster* monster = creature->getMonster()) {
		if (const Tile* tile = g_game.map.getTile(toPos); tile && !monster->isSummon()) {
			if (Creature* topCreature = tile->getTopCreature()) {
				if (Player* tilePlayer = topCreature->getPlayer()) {
					if (tilePlayer->isInvisible() && !monster->canSeeInvisibility() ||
						tilePlayer->hasFlag(PlayerFlag_CannotBeAttacked)) {
						return;
					}

					if (!monster->isCreatureAvoidable(tilePlayer) && monster->isOpponent(tilePlayer) && monster->canPushCreatures() && tilePlayer != monster->getAttackedCreature()) {
						monster->setAttackedCreature(nullptr);
						monster->clearToDo();
						monster->addWaitToDo(1000);
						monster->startToDo();
					}

					return;
				}
			}
		}
	}

	creature->onWalk(direction, flags);

	const ReturnValue ret = internalMoveCreature(creature, direction, flags);
	if (ret == RETURNVALUE_NOERROR) {
		return;
	}
  
  	if (Npc* npc = creature->getNpc()) {
		if (!npc->canWalkTo(creature->getPosition(), direction)) {
			npc->clearToDo();
			npc->addYieldToDo();
			return;
		}
	}

	// this situation may happen with auto walking or when you are paralyzed
	if (Player* player = creature->getPlayer()) {
		player->sendCancelWalk();

		if (player->clearToDo() && !player->attackedCreature) {
			creature->stopToDo();
		}

		player->sendCancelMessage(ret);
		
		player->addYieldToDo();
	}
}

ReturnValue Game::internalMoveCreature(Creature* creature, Direction direction, uint32_t flags /*= 0*/)
{
	creature->setLastPosition(creature->getPosition());
	const Position& currentPos = creature->getPosition();
	Position destPos = getNextPosition(direction, currentPos);
	Player* player = creature->getPlayer();

	bool diagonalMovement = (direction & DIRECTION_DIAGONAL_MASK) != 0;
	if (player && !diagonalMovement) {
		//try to go up
		if (currentPos.z != 8 && creature->getTile()->hasHeight(3)) {
			Tile* tmpTile = map.getTile(currentPos.x, currentPos.y, currentPos.getZ() - 1);
			if (tmpTile == nullptr || (tmpTile->getGround() == nullptr && !tmpTile->hasFlag(TILESTATE_BLOCKSOLID))) {
				tmpTile = map.getTile(destPos.x, destPos.y, destPos.getZ() - 1);
				if (tmpTile && tmpTile->getGround() && !tmpTile->hasFlag(TILESTATE_BLOCKSOLID) && !tmpTile->hasHeight(1) && !tmpTile->getBottomCreature()) {
					flags |= FLAG_IGNOREBLOCKITEM | FLAG_IGNOREBLOCKCREATURE;

					if (!tmpTile->hasFlag(TILESTATE_FLOORCHANGE)) {
						player->setDirection(direction);
						destPos.z--;
					}
				}
			}
		}

		//try to go down
		if (currentPos.z != 7 && currentPos.z == destPos.z) {
			Tile* tmpTile = map.getTile(destPos.x, destPos.y, destPos.z);
			if (tmpTile == nullptr || (tmpTile->getGround() == nullptr && !tmpTile->hasFlag(TILESTATE_BLOCKSOLID))) {
				tmpTile = map.getTile(destPos.x, destPos.y, destPos.z + 1);
				if (tmpTile && tmpTile->hasHeight(3) && !tmpTile->hasFlag(TILESTATE_BLOCKSOLID) && !tmpTile->getBottomCreature()) {
					flags |= FLAG_IGNOREBLOCKITEM | FLAG_IGNOREBLOCKCREATURE;
					player->setDirection(direction);
					destPos.z++;
				}
			}
		}
	}

	Tile* toTile = map.getTile(destPos);
	if (!toTile) {
		return RETURNVALUE_NOTPOSSIBLE;
	}
	return internalMoveCreature(*creature, *toTile, flags);
}

ReturnValue Game::internalMoveCreature(Creature& creature, Tile& toTile, uint32_t flags /*= 0*/)
{
	//check if we can move the creature to the destination
	ReturnValue ret = toTile.queryAdd(0, creature, 1, flags);
	if (ret != RETURNVALUE_NOERROR) {
		if ((ret == RETURNVALUE_NOTENOUGHROOM || ret == RETURNVALUE_PLAYERISNOTINVITED) && creature.getTile()->hasHeight(3)) {
			if (ret == RETURNVALUE_PLAYERISNOTINVITED && !toTile.hasProperty(ITEMPROPERTY::CONST_PROP_BLOCKSOLID)) {
				return ret;
			}
			return RETURNVALUE_NOTPOSSIBLE;
		}
		return ret;
	}

	Position fromPos = creature.getPosition();

	if (Monster* monster = creature.getMonster()) {
		if (monster->canPushItems()) {
			Monster::pushItems(fromPos, &toTile);
		}

		if (monster->canPushCreatures()) {
			if (!Monster::pushCreatures(fromPos, &toTile, monster)) {
				// we could not push the creature, or we killed it, so we can't move
				return RETURNVALUE_NOTPOSSIBLE;
			}
		}
	}

	map.moveCreature(creature, toTile);

	if (creature.getParent() != &toTile) {
		return RETURNVALUE_NOERROR;
	}

	int32_t index = 0;
	Item* toItem = nullptr;
	Tile* subCylinder = nullptr;
	Tile* toCylinder = &toTile;
	Tile* fromCylinder = nullptr;
	uint32_t n = 0;

	while ((subCylinder = toCylinder->queryDestination(index, creature, &toItem, flags)) != toCylinder) {
		map.moveCreature(creature, *subCylinder);

		if (creature.getParent() != subCylinder) {
			//could happen if a script move the creature
			fromCylinder = nullptr;
			break;
		}

		fromCylinder = toCylinder;
		toCylinder = subCylinder;
		flags = 0;

		//to prevent infinite loop
		if (++n >= MAP_MAX_LAYERS) {
			break;
		}
	}

	if (fromCylinder) {
		const Position& fromPosition = fromCylinder->getPosition();
		const Position& toPosition = toCylinder->getPosition();
		if (fromPosition.z != toPosition.z && (fromPosition.x != toPosition.x || fromPosition.y != toPosition.y)) {
			Direction dir = getDirectionTo(fromPosition, toPosition);
			if ((dir & DIRECTION_DIAGONAL_MASK) == 0) {
				internalCreatureTurn(&creature, dir);
			}
		}
	}

	return RETURNVALUE_NOERROR;
}

void Game::playerMoveItemByPlayerID(uint32_t playerId, const Position fromPos, uint16_t spriteId, uint8_t fromStackPos, const Position toPos, uint8_t count)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	playerMoveItem(player, fromPos, spriteId, fromStackPos, toPos, count, nullptr, nullptr);
}

void Game::playerMoveItem(Player* player, const Position fromPos,
                          uint16_t spriteId, uint8_t fromStackPos, const Position toPos, uint8_t count, Item* item, Cylinder* toCylinder)
{
	if (item == nullptr) {
		uint8_t fromIndex = 0;
		if (fromPos.x == 0xFFFF) {
			if (fromPos.y & 0x40) {
				fromIndex = fromPos.z;
			} else {
				fromIndex = static_cast<uint8_t>(fromPos.y);
			}
		} else {
			fromIndex = fromStackPos;
		}

		Thing* thing = internalGetThing(player, fromPos, fromIndex, 0, STACKPOS_MOVE);
		if (!thing || !thing->getItem()) {
			player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
			return;
		}

		item = thing->getItem();
	}

	if (item->getClientID() != spriteId) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		return;
	}

	Cylinder* fromCylinder = internalGetCylinder(player, fromPos);
	if (fromCylinder == nullptr) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		return;
	}

	if (toCylinder == nullptr) {
		toCylinder = internalGetCylinder(player, toPos);
		if (toCylinder == nullptr) {
			player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
			return;
		}
	}

	if (!item->isPushable() || item->hasAttribute(ITEM_ATTRIBUTE_UNIQUEID)) {
		player->sendCancelMessage(RETURNVALUE_NOTMOVEABLE);
		return;
	}

	const Position& playerPos = player->getPosition();
	const Position& mapFromPos = fromCylinder->getTile()->getPosition();
	if (playerPos.z != mapFromPos.z) {
		player->sendCancelMessage(playerPos.z > mapFromPos.z ? RETURNVALUE_FIRSTGOUPSTAIRS : RETURNVALUE_FIRSTGODOWNSTAIRS);
		return;
	}

	if (!Position::areInRange<1, 1>(playerPos, mapFromPos)) {
		//need to walk to the item first before using it
		std::vector<Direction> listDir;
		if (player->getPathTo(item->getPosition(), listDir, 0, 1, true, true)) {
			player->addWalkToDo(listDir);
			player->addWaitToDo(100);
			player->addActionToDo(std::bind(&Game::playerMoveItemByPlayerID, this,
				player->getID(), fromPos, spriteId, fromStackPos, toPos, count));
			player->startToDo();
		} else {
			player->sendCancelMessage(RETURNVALUE_THEREISNOWAY);
		}
		return;
	}

	const Tile* toCylinderTile = toCylinder->getTile();
	const Position& mapToPos = toCylinderTile->getPosition();

	//hangable item specific code
	if (item->isHangable() && toCylinderTile->hasFlag(TILESTATE_SUPPORTS_HANGABLE)) {
		//destination supports hangable objects so need to move there first
		bool vertical = toCylinderTile->hasProperty(CONST_PROP_ISVERTICAL);
		if (vertical) {
			if (playerPos.x + 1 == mapToPos.x) {
				player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
				return;
			}
		} else { // horizontal
			if (playerPos.y + 1 == mapToPos.y) {
				player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
				return;
			}
		}

		if (!Position::areInRange<1, 1, 0>(playerPos, mapToPos)) {
			Position walkPos = mapToPos;
			if (vertical) {
				walkPos.x++;
			} else {
				walkPos.y++;
			}

			Position itemPos = fromPos;
			uint8_t itemStackPos = fromStackPos;

			if (fromPos.x != 0xFFFF && Position::areInRange<1, 1>(mapFromPos, playerPos)
			        && !Position::areInRange<1, 1, 0>(mapFromPos, walkPos)) {
				//need to pickup the item first
				Item* moveItem = nullptr;

				ReturnValue ret = internalMoveItem(fromCylinder, player, INDEX_WHEREEVER, item, count, &moveItem, 0, player, nullptr, &fromPos, &toPos);
				if (ret != RETURNVALUE_NOERROR) {
					player->sendCancelMessage(ret);
					return;
				}

				//changing the position since its now in the inventory of the player
				internalGetPosition(moveItem, itemPos, itemStackPos);
			}

			std::vector<Direction> listDir;
			if (player->getPathTo(walkPos, listDir, 0, 0, true, true)) {
				player->addWalkToDo(listDir);
				player->addWaitToDo(100);
				player->addActionToDo(std::bind(&Game::playerMoveItemByPlayerID, this,
					player->getID(), itemPos, spriteId, itemStackPos, toPos, count));
				player->startToDo();
			} else {
				player->sendCancelMessage(RETURNVALUE_THEREISNOWAY);
			}
			return;
		}
	}

	if (!item->isPickupable() && playerPos.z != mapToPos.z) {
		player->sendCancelMessage(RETURNVALUE_DESTINATIONOUTOFREACH);
		return;
	}

	int32_t throwRange = item->getThrowRange();
	if ((Position::getDistanceX(playerPos, mapToPos) > throwRange) ||
	        (Position::getDistanceY(playerPos, mapToPos) > throwRange)) {
		player->sendCancelMessage(RETURNVALUE_DESTINATIONOUTOFREACH);
		return;
	}

	// hangable logic is handled up
	if (!(item->isHangable() && toCylinderTile->hasFlag(TILESTATE_SUPPORTS_HANGABLE))) {
		if (toPos.x != 0xFFFF && !canThrowObjectTo(item->getPosition(), mapToPos, true)) {
			player->sendCancelMessage(RETURNVALUE_CANNOTTHROW);
			return;
		}
	}

	uint8_t toIndex = 0;
	if (toPos.x == 0xFFFF) {
		if (toPos.y & 0x40) {
			toIndex = toPos.z;
		} else {
			toIndex = static_cast<uint8_t>(toPos.y);
		}
	}

	const Item* previouslyEquippedWeapon = player->getWeapon(true);

	ReturnValue ret = internalMoveItem(fromCylinder, toCylinder, toIndex, item, count, nullptr, 0, player, nullptr, &fromPos, &toPos);
	if (ret != RETURNVALUE_NOERROR) {
		player->sendCancelMessage(ret);
		return;
	}

	// delay attacks when switching weapons
	if (previouslyEquippedWeapon != player->getWeapon(true)) {
		if (toCylinder == player) {
			player->earliestAttackTime = OTSYS_TIME() + player->getAttackSpeed();
		}
	}
}

ReturnValue Game::internalMoveItem(Cylinder* fromCylinder, Cylinder* toCylinder, int32_t index,
                                   Item* item, uint32_t count, Item** _moveItem, uint32_t flags /*= 0*/, Creature* actor/* = nullptr*/, Item* tradeItem/* = nullptr*/, const Position* fromPos /*= nullptr*/, const Position* toPos/*= nullptr*/)
{
	Player* actorPlayer = actor ? actor->getPlayer() : nullptr;
	if (actorPlayer && fromPos && toPos) {
		ReturnValue ret = g_events->eventPlayerOnMoveItem(actorPlayer, item, count, *fromPos, *toPos, fromCylinder, toCylinder);
		if (ret != RETURNVALUE_NOERROR) {
			return ret;
		}
	}

	Item* toItem = nullptr;

	Cylinder* subCylinder;
	int floorN = 0;

	while ((subCylinder = toCylinder->queryDestination(index, *item, &toItem, flags)) != toCylinder) {
		toCylinder = subCylinder;

		// Fixes https://github.com/Ezzz-dev/TheVioletProject/issues/111
		if (!toCylinder->getTile()) {
			flags = 0;
		} 

		//to prevent infinite loop
		if (++floorN >= MAP_MAX_LAYERS) {
			break;
		}
	}

	//destination is the same as the source?
	if (item == toItem) {
		return RETURNVALUE_NOERROR; //silently ignore move
	}

	//check if we can add this item
	ReturnValue ret = toCylinder->queryAdd(index, *item, count, flags, actor);
	if (ret == RETURNVALUE_NEEDEXCHANGE) {
		//check if we can add it to source cylinder
		if (Container* fromContainer = fromCylinder->getContainer()) {
			if (fromContainer->size() == fromContainer->capacity()) {
				return RETURNVALUE_CONTAINERNOTENOUGHROOM;
			}
		}

		ret = fromCylinder->queryAdd(fromCylinder->getThingIndex(item), *toItem, toItem->getItemCount(), 0);
		if (ret == RETURNVALUE_NOERROR) {
			if (actorPlayer && fromPos && toPos && g_events->eventPlayerOnMoveItem(actorPlayer, toItem, toItem->getItemCount(), *toPos, *fromPos, toCylinder, fromCylinder) != RETURNVALUE_NOERROR) {
				return RETURNVALUE_NOTPOSSIBLE;
			}

			//check how much we can move
			uint32_t maxExchangeQueryCount = 0;
			ReturnValue retExchangeMaxCount = fromCylinder->queryMaxCount(INDEX_WHEREEVER, *toItem, toItem->getItemCount(), maxExchangeQueryCount, 0);

			if (retExchangeMaxCount != RETURNVALUE_NOERROR && maxExchangeQueryCount == 0) {
				return retExchangeMaxCount;
			}

			ret = fromCylinder->queryRemove(*item, count, flags, actor);
			if (ret != RETURNVALUE_NOERROR) {
				return ret;
			}

			if (toCylinder->queryRemove(*toItem, toItem->getItemCount(), flags, actor) == RETURNVALUE_NOERROR) {
				int32_t oldToItemIndex = toCylinder->getThingIndex(toItem);
				toCylinder->removeThing(toItem, toItem->getItemCount());
				fromCylinder->addThing(toItem);

				if (oldToItemIndex != -1) {
					toCylinder->postRemoveNotification(toItem, fromCylinder, oldToItemIndex);
				}

				int32_t newToItemIndex = fromCylinder->getThingIndex(toItem);
				if (newToItemIndex != -1) {
					fromCylinder->postAddNotification(toItem, toCylinder, newToItemIndex);
				}

				if (actorPlayer && fromPos && toPos && toItem) {
					g_events->eventPlayerOnItemMoved(actorPlayer, item, count, *fromPos, *toPos, fromCylinder, toCylinder);
					g_events->eventPlayerOnItemMoved(actorPlayer, toItem, count, *fromPos, *toPos, fromCylinder, toCylinder);
				}

				ret = toCylinder->queryAdd(index, *item, count, flags);
				toItem = nullptr;
			}
		}
	}

	if (g_config.getBoolean(ConfigManager::CLASSIC_INVENTORY_SWAP)) {
		if ((ret == RETURNVALUE_NOTENOUGHCAPACITY && toCylinder->getCreature() || ret == RETURNVALUE_BOTHHANDSNEEDTOBEFREE || ret == RETURNVALUE_CANONLYUSEONEWEAPON) && toItem) {
			if (item->equals(toItem) && item->isStackable()) {
				return ret;
			}

			if (fromCylinder->queryAdd(fromCylinder->getThingIndex(item), *toItem, toItem->getItemCount(), 0) == RETURNVALUE_NOERROR) {
				toCylinder->removeThing(toItem, toItem->getItemCount());
				fromCylinder->addThing(toItem);

				Player* player = toCylinder->getCreature()->getPlayer(); // we are assured it is a player
				player->updateInventoryWeight();

				if (toCylinder->queryAdd(index, *item, count, 0) == RETURNVALUE_NOERROR) {
					fromCylinder->removeThing(item, count);

					if (!item->isRemoved() && item->isStackable()) {
						toCylinder->addThing(index, Item::CreateItem(item->getID(), count));
					} else {
						toCylinder->addThing(index, item);
					}
					ret = RETURNVALUE_NOERROR;
				}

				player->updateInventoryWeight();
				player->sendStats();
			}
			return ret;
		}
	}

	if (ret != RETURNVALUE_NOERROR) {
		return ret;
	}

	//check how much we can move
	uint32_t maxQueryCount = 0;
	ReturnValue retMaxCount = toCylinder->queryMaxCount(index, *item, count, maxQueryCount, flags);
	if (retMaxCount != RETURNVALUE_NOERROR && maxQueryCount == 0) {
		return retMaxCount;
	}

	uint32_t m;
	if (item->isStackable()) {
		m = std::min<uint32_t>(count, maxQueryCount);
	} else {
		m = maxQueryCount;
	}

	Item* moveItem = item;

	//check if we can remove this item
	ret = fromCylinder->queryRemove(*item, m, flags, actor);
	if (ret != RETURNVALUE_NOERROR) {
		return ret;
	}

	if (tradeItem) {
		if (toCylinder->getItem() == tradeItem) {
			return RETURNVALUE_NOTENOUGHROOM;
		}

		Cylinder* tmpCylinder = toCylinder->getParent();
		while (tmpCylinder) {
			if (tmpCylinder->getItem() == tradeItem) {
				return RETURNVALUE_NOTENOUGHROOM;
			}

			tmpCylinder = tmpCylinder->getParent();
		}
	}

	//remove the item
	int32_t itemIndex = fromCylinder->getThingIndex(item);
	Item* updateItem = nullptr;
	fromCylinder->removeThing(item, m);

	//update item(s)
	if (item->isStackable()) {
		uint32_t n;

		if (item->equals(toItem)) {
			n = std::min<uint32_t>(100 - toItem->getItemCount(), m);

			if (g_config.getBoolean(ConfigManager::TILE_OLDSCHOOL_ITEM_STACKING)) {
				if (toCylinder->getTile()) {
					if (toItem->getItemCount() + m > 100) {
						n = 0;
					}
				}
			}

			toCylinder->updateThing(toItem, toItem->getID(), toItem->getItemCount() + n);
			updateItem = toItem;
		} else {
			n = 0;
		}

		int32_t newCount = m - n;
		if (newCount > 0) {
			moveItem = item->clone();
			moveItem->setItemCount(newCount);
		} else {
			moveItem = nullptr;
		}

		if (item->isRemoved()) {
			ReleaseItem(item);
		}
	}

	//add item
	if (moveItem /*m - n > 0*/) {
		toCylinder->addThing(index, moveItem);
	}

	if (itemIndex != -1) {
		fromCylinder->postRemoveNotification(item, toCylinder, itemIndex);
	}

	if (moveItem) {
		int32_t moveItemIndex = toCylinder->getThingIndex(moveItem);
		if (moveItemIndex != -1) {
			toCylinder->postAddNotification(moveItem, fromCylinder, moveItemIndex);
		}
	}

	if (updateItem) {
		int32_t updateItemIndex = toCylinder->getThingIndex(updateItem);
		if (updateItemIndex != -1) {
			toCylinder->postAddNotification(updateItem, fromCylinder, updateItemIndex);
		}
	}

	if (_moveItem) {
		if (moveItem) {
			*_moveItem = moveItem;
		} else {
			*_moveItem = item;
		}
	}

	//we could not move all, inform the player
	if (item->isStackable() && maxQueryCount < count) {
		return retMaxCount;
	}

	if (moveItem && moveItem->getDuration() > 0) {
		if (moveItem->getDecaying() != DECAYING_TRUE) {
			moveItem->incrementReferenceCounter();
			moveItem->setDecaying(DECAYING_TRUE);
			toDecayItems.push_front(moveItem);
		}
	}

	if (actorPlayer && fromPos && toPos) {
		g_events->eventPlayerOnItemMoved(actorPlayer, item, count, *fromPos, *toPos, fromCylinder, toCylinder);
	}

	return ret;
}

ReturnValue Game::internalAddItem(Cylinder* toCylinder, Item* item, int32_t index /*= INDEX_WHEREEVER*/,
                                  uint32_t flags/* = 0*/, bool test/* = false*/)
{
	uint32_t remainderCount = 0;
	return internalAddItem(toCylinder, item, index, flags, test, remainderCount);
}

ReturnValue Game::internalAddItem(Cylinder* toCylinder, Item* item, int32_t index,
                                  uint32_t flags, bool test, uint32_t& remainderCount)
{
	if (toCylinder == nullptr || item == nullptr) {
		return RETURNVALUE_NOTPOSSIBLE;
	}

	Cylinder* destCylinder = toCylinder;
	Item* toItem = nullptr;
	toCylinder = toCylinder->queryDestination(index, *item, &toItem, flags);

	//check if we can add this item
	ReturnValue ret = toCylinder->queryAdd(index, *item, item->getItemCount(), flags);
	if (ret != RETURNVALUE_NOERROR) {
		return ret;
	}

	/*
	Check if we can move add the whole amount, we do this by checking against the original cylinder,
	since the queryDestination can return a cylinder that might only hold a part of the full amount.
	*/
	uint32_t maxQueryCount = 0;
	ret = destCylinder->queryMaxCount(INDEX_WHEREEVER, *item, item->getItemCount(), maxQueryCount, flags);

	if (ret != RETURNVALUE_NOERROR) {
		return ret;
	}

	if (test) {
		return RETURNVALUE_NOERROR;
	}

	if (item->isStackable() && item->equals(toItem)) {
		uint32_t m = std::min<uint32_t>(item->getItemCount(), maxQueryCount);
		uint32_t n = std::min<uint32_t>(100 - toItem->getItemCount(), m);

		toCylinder->updateThing(toItem, toItem->getID(), toItem->getItemCount() + n);

		int32_t count = m - n;
		if (count > 0) {
			if (item->getItemCount() != count) {
				Item* remainderItem = item->clone();
				remainderItem->setItemCount(count);
				if (internalAddItem(destCylinder, remainderItem, INDEX_WHEREEVER, flags, false) != RETURNVALUE_NOERROR) {
					ReleaseItem(remainderItem);
					remainderCount = count;
				}
			} else {
				toCylinder->addThing(index, item);

				int32_t itemIndex = toCylinder->getThingIndex(item);
				if (itemIndex != -1) {
					toCylinder->postAddNotification(item, nullptr, itemIndex);
				}
			}
		} else {
			//fully merged with toItem, item will be destroyed
			item->onRemoved();
			ReleaseItem(item);

			int32_t itemIndex = toCylinder->getThingIndex(toItem);
			if (itemIndex != -1) {
				toCylinder->postAddNotification(toItem, nullptr, itemIndex);
			}
		}
	} else {
		toCylinder->addThing(index, item);

		int32_t itemIndex = toCylinder->getThingIndex(item);
		if (itemIndex != -1) {
			toCylinder->postAddNotification(item, nullptr, itemIndex);
		}
	}

	if (item->getDuration() > 0) {
		item->incrementReferenceCounter();
		item->setDecaying(DECAYING_TRUE);
		toDecayItems.push_front(item);
	}

	return RETURNVALUE_NOERROR;
}

ReturnValue Game::internalRemoveItem(Item* item, int32_t count /*= -1*/, bool test /*= false*/, uint32_t flags /*= 0*/)
{
	Cylinder* cylinder = item->getParent();
	if (cylinder == nullptr) {
		return RETURNVALUE_NOTPOSSIBLE;
	}

	if (count == -1) {
		count = item->getItemCount();
	}

	//check if we can remove this item
	ReturnValue ret = cylinder->queryRemove(*item, count, flags | FLAG_IGNORENOTMOVEABLE);
	if (ret != RETURNVALUE_NOERROR) {
		return ret;
	}

	if (!test) {
		//remove bed from houses
		if (Tile* tile = dynamic_cast<Tile*>(cylinder)) {
			if (House* house = tile->getHouse()) {
				if (Item::items[item->getID()].isBed()) {
					house->removeBed(item->getBed());
				}
			}
		}

		int32_t index = cylinder->getThingIndex(item);

		//remove the item
		cylinder->removeThing(item, count);

		if (item->isRemoved()) {
			item->onRemoved();
			if (item->canDecay()) {
				decayItems->remove(item);
			}
			ReleaseItem(item);
		}

		cylinder->postRemoveNotification(item, nullptr, index);
	}

	return RETURNVALUE_NOERROR;
}

ReturnValue Game::internalPlayerAddItem(Player* player, Item* item, bool dropOnMap /*= true*/, slots_t slot /*= CONST_SLOT_WHEREEVER*/)
{
	ReturnValue ret = RETURNVALUE_NOTPOSSIBLE;
	uint32_t remainderCount = 0;
	if (slot == CONST_SLOT_WHEREEVER) {
		bool first = true;

		auto* event = g_moveEvents->getItemIdEvent(item, MoveEvent_t::MOVE_EVENT_ADD_ITEM_ITEMTILE);
		if (!event) {
			event = g_moveEvents->getItemIdEvent(item, MoveEvent_t::MOVE_EVENT_REMOVE_ITEM_ITEMTILE);
			if (!event) {
				event = g_moveEvents->getItemIdEvent(item, MoveEvent_t::MOVE_EVENT_REMOVE_ITEM);
				if (!event) {
					event = g_moveEvents->getItemIdEvent(item, MoveEvent_t::MOVE_EVENT_ADD_ITEM);
				}
			}
		}

		const bool invert = event != nullptr || Item::items[item->getID()].transformEquipTo != 0;

		// Find a suitable slot
		for (int32_t i = 0; i <= 1; i++) {
			for (int32_t nextSlot = CONST_SLOT_HEAD; nextSlot <= CONST_SLOT_LAST; nextSlot++) {
				if (first != invert) {
					if (!player->inventory[nextSlot] && Item::items[item->getID()].slotPosition & getSlotPositionBits(nextSlot) &&
						player->queryAdd(nextSlot, *item, item->getItemCount(), FLAG_NOLIMIT) == RETURNVALUE_NOERROR) {
						slot = static_cast<slots_t>(nextSlot);
						i = 2;
						break;
					}
				}

				bool placeHere = false;
				if (first == invert) {
					if (player->inventory[nextSlot] && player->inventory[nextSlot]->getContainer()) {
						placeHere = true;
					}
				}

				if (placeHere) {
					ret = internalAddItem(player, item, nextSlot, 0, false, remainderCount);
					if (remainderCount != 0) {
						Item* remainderItem = Item::CreateItem(item->getID(), remainderCount);
						ReturnValue remaindRet = internalAddItem(player->getTile(), remainderItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
						if (remaindRet != RETURNVALUE_NOERROR) {
							ReleaseItem(remainderItem);
						}
					}

					if (ret == RETURNVALUE_NOERROR) {
						g_moveEvents->onItemMove(item, player->getTile(), true);
						return ret;
					}
				}
			}

			first = false;
		}
	}

	ret = internalAddItem(player, item, slot, 0, false, remainderCount);
	if (remainderCount != 0) {
		Item* remainderItem = Item::CreateItem(item->getID(), remainderCount);
		ReturnValue remaindRet = internalAddItem(player->getTile(), remainderItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
		if (remaindRet != RETURNVALUE_NOERROR) {
			ReleaseItem(remainderItem);
		}
	}

	if (ret != RETURNVALUE_NOERROR && dropOnMap) {
		ret = internalAddItem(player->getTile(), item, INDEX_WHEREEVER, FLAG_NOLIMIT);
	}

	if (ret == RETURNVALUE_NOERROR) {
		g_moveEvents->onItemMove(item, player->getTile(), true);
	}

	return ret;
}

Item* Game::findItemOfType(Cylinder* cylinder, uint16_t itemId,
                           bool depthSearch /*= true*/, int32_t subType /*= -1*/) const
{
	if (cylinder == nullptr) {
		return nullptr;
	}

	std::vector<Container*> containers;
	for (size_t i = cylinder->getFirstIndex(), j = cylinder->getLastIndex(); i < j; ++i) {
		Thing* thing = cylinder->getThing(i);
		if (!thing) {
			continue;
		}

		Item* item = thing->getItem();
		if (!item) {
			continue;
		}

		if (item->getID() == itemId && (subType == -1 || subType == item->getSubType())) {
			return item;
		}

		if (depthSearch) {
			Container* container = item->getContainer();
			if (container) {
				containers.push_back(container);
			}
		}
	}

	size_t i = 0;
	while (i < containers.size()) {
		Container* container = containers[i++];
		for (Item* item : container->getItemList()) {
			if (item->getID() == itemId && (subType == -1 || subType == item->getSubType())) {
				return item;
			}

			Container* subContainer = item->getContainer();
			if (subContainer) {
				containers.push_back(subContainer);
			}
		}
	}
	return nullptr;
}

bool Game::removeMoney(Cylinder* cylinder, uint64_t money, uint32_t flags /*= 0*/)
{
	if (cylinder == nullptr) {
		return false;
	}

	if (money == 0) {
		return true;
	}

	std::vector<Container*> containers;

	std::multimap<uint32_t, Item*> moneyMap;
	uint64_t moneyCount = 0;

	for (size_t i = cylinder->getFirstIndex(), j = cylinder->getLastIndex(); i < j; ++i) {
		Thing* thing = cylinder->getThing(i);
		if (!thing) {
			continue;
		}

		Item* item = thing->getItem();
		if (!item) {
			continue;
		}

		Container* container = item->getContainer();
		if (container) {
			containers.push_back(container);
		} else {
			const uint32_t worth = item->getWorth();
			if (worth != 0) {
				moneyCount += worth;
				moneyMap.emplace(worth, item);
			}
		}
	}

	size_t i = 0;
	while (i < containers.size()) {
		Container* container = containers[i++];
		for (Item* item : container->getItemList()) {
			Container* tmpContainer = item->getContainer();
			if (tmpContainer) {
				containers.push_back(tmpContainer);
			} else {
				const uint32_t worth = item->getWorth();
				if (worth != 0) {
					moneyCount += worth;
					moneyMap.emplace(worth, item);
				}
			}
		}
	}

	if (moneyCount < money) {
		return false;
	}

	for (const auto& moneyEntry : moneyMap) {
		Item* item = moneyEntry.second;
		if (moneyEntry.first < money) {
			internalRemoveItem(item);
			money -= moneyEntry.first;
		} else if (moneyEntry.first > money) {
			const uint32_t worth = moneyEntry.first / item->getItemCount();
			const uint32_t removeCount = std::ceil(money / static_cast<double>(worth));

			internalRemoveItem(item, removeCount, false, flags);
			addMoney(cylinder, (worth * removeCount) - money, flags);
			break;
		} else {
			internalRemoveItem(item);
			break;
		}
	}
	return true;
}

bool calculateChange(uint64_t money, int32_t& gold, int32_t& platinum, int32_t& crystal)
{
	uint64_t platinumWorth = 100 * platinum;
	if (gold + (10000 * crystal) + platinumWorth < money) {
		return false;
	}

	int32_t crystals = money / 10000;
	int32_t platinums = money % 10000;
	int32_t goldCopy = gold;
	if (platinumWorth + goldCopy >= money % 10000) {
		int32_t totalCrystals = 0;
		if (crystal < crystals) {
			totalCrystals = money - 10000 * crystal;
		} else {
			totalCrystals = money % 10000;
			crystal = crystals;
			goldCopy = gold;
		}

		int32_t totalGold = totalCrystals / 100;
		int32_t value = totalCrystals % 100;

		if (goldCopy >= totalCrystals % 100) {
			if (platinum < totalGold) {
				gold = totalCrystals - 100 * platinum;
			} else {
				platinum = totalGold;
				gold = value;
			}
		} else {
			platinum = totalGold + 1;
			gold = value - 100;
		}
	} else {
		crystal = crystals + 1;
		platinum = (platinums - 10000) / 100;
		gold = (platinums - 10000) % 100;
	}

	if (gold + (10000 * crystal) + 100 * platinum != money) {
		return false;
	}

	return true;
}

int32_t deleteCoins(Cylinder* cylinder, uint16_t id, int32_t money)
{
	for (int32_t i = cylinder->getFirstIndex(); i < cylinder->getLastIndex() && money > 0; i++) {
		if (!cylinder->getThing(i)) {
			continue;
		}

		Item* item = cylinder->getThing(i)->getItem();
		if (item) {
			if (Container* container = item->getContainer()) {
				money = deleteCoins(container, id, money);
				if (!money) {
					return 0;
				}
			} else if (item->getID() == id) {
				if (item->getItemCount() > money) {
					g_game.transformItem(item, item->getID(), item->getItemCount() - money);
					return 0;
				}

				money -= item->getItemCount();
				g_game.internalRemoveItem(item, item->getItemCount());
			}
		}
	}

	return money;
}

bool Game::removePlayerMoney(Player* player, uint64_t money)
{
	int32_t gold = player->getItemTypeCount(ITEM_GOLD_COIN);
	int32_t platinum = player->getItemTypeCount(ITEM_PLATINUM_COIN);
	int32_t crystal = player->getItemTypeCount(ITEM_CRYSTAL_COIN);

	if (!calculateChange(money, gold, platinum, crystal)) {
		return false;
	}

	if (gold > 0) {
		gold = deleteCoins(player, ITEM_GOLD_COIN, gold);
	}

	if (gold < 0) {
		Item* item = Item::CreateItem(ITEM_GOLD_COIN, -gold);
		g_game.internalPlayerAddItem(player, item);
	}

	if (platinum > 0) {
		gold = deleteCoins(player, ITEM_PLATINUM_COIN, platinum);
	}

	if (platinum < 0) {
		Item* item = Item::CreateItem(ITEM_PLATINUM_COIN, -platinum);
		g_game.internalPlayerAddItem(player, item);
	}

	if (crystal > 0) {
		deleteCoins(player, ITEM_CRYSTAL_COIN, crystal);
	}

	return true;
}

void Game::addMoney(Cylinder* cylinder, uint64_t money, uint32_t flags /*= 0*/)
{
	if (money == 0) {
		return;
	}

	std::list<Item*> remainders;

	uint32_t crystalCoins = money / 10000;
	money -= crystalCoins * 10000;
	while (crystalCoins > 0) {
		const uint16_t count = std::min<uint32_t>(100, crystalCoins);

		Item* remaindItem = Item::CreateItem(ITEM_CRYSTAL_COIN, count);
		remainders.push_front(remaindItem);

		crystalCoins -= count;
	}

	uint16_t platinumCoins = money / 100;
	if (platinumCoins != 0) {
		Item* remaindItem = Item::CreateItem(ITEM_PLATINUM_COIN, platinumCoins);
		remainders.push_front(remaindItem);

		money -= platinumCoins * 100;
	}

	if (money != 0) {
		Item* remaindItem = Item::CreateItem(ITEM_GOLD_COIN, money);
		remainders.push_front(remaindItem);
	}

	for (Item* remainder : remainders) {
		ReturnValue ret = internalAddItem(cylinder, remainder, INDEX_WHEREEVER, flags);
		if (ret != RETURNVALUE_NOERROR) {
			internalAddItem(cylinder->getTile(), remainder, INDEX_WHEREEVER, FLAG_NOLIMIT);
		}
	}
}

Item* Game::transformItem(Item* item, uint16_t newId, int32_t newCount /*= -1*/)
{
	if (item->getID() == newId && (newCount == -1 || (newCount == item->getSubType() && newCount != 0))) { //chargeless item placed on map = infinite
		return item;
	}

	Cylinder* cylinder = item->getParent();
	if (cylinder == nullptr) {
		return nullptr;
	}

	int32_t itemIndex = cylinder->getThingIndex(item);
	if (itemIndex == -1) {
		return item;
	}

	if (!item->canTransform()) {
		return item;
	}

	const ItemType& newType = Item::items[newId];
	if (newType.id == 0) {
		return item;
	}

	const ItemType& curType = Item::items[item->getID()];
	if (curType.alwaysOnTop != newType.alwaysOnTop) {
		//This only occurs when you transform items on tiles from a downItem to a topItem (or vice versa)
		//Remove the old, and add the new
		cylinder->removeThing(item, item->getItemCount());
		cylinder->postRemoveNotification(item, cylinder, itemIndex);

		item->setID(newId);
		if (newCount != -1) {
			item->setSubType(newCount);
		}
		cylinder->addThing(item);

		Cylinder* newParent = item->getParent();
		if (newParent == nullptr) {
			ReleaseItem(item);
			return nullptr;
		}

		newParent->postAddNotification(item, cylinder, newParent->getThingIndex(item));
		return item;
	}

	if (curType.type == newType.type) {
		//Both items has the same type so we can safely change id/subtype
		if (newCount == 0 && (item->isStackable() || item->hasAttribute(ITEM_ATTRIBUTE_CHARGES))) {
			if (item->isStackable()) {
				internalRemoveItem(item);
				return nullptr;
			} else {
				int32_t newItemId = newId;
				if (curType.id == newType.id) {
					newItemId = item->getDecayTo();
				}

				if (newItemId < 0) {
					internalRemoveItem(item);
					return nullptr;
				} else if (newItemId != newId) {
					//Replacing the the old item with the new while maintaining the old position
					Item* newItem = Item::CreateItem(newItemId, 1);
					if (newItem == nullptr) {
						return nullptr;
					}

					cylinder->replaceThing(itemIndex, newItem);
					cylinder->postAddNotification(newItem, cylinder, itemIndex);

					item->setParent(nullptr);
					cylinder->postRemoveNotification(item, cylinder, itemIndex);
					ReleaseItem(item);
					return newItem;
				} else {
					return transformItem(item, newItemId);
				}
			}
		} else {
			cylinder->postRemoveNotification(item, cylinder, itemIndex);
			uint16_t itemId = item->getID();
			int32_t count = item->getSubType();

			if (curType.id != newType.id) {
				if (newType.group != curType.group) {
					item->setDefaultSubtype();
				}

				itemId = newId;
			}

			if (newCount != -1 && newType.hasSubType()) {
				count = newCount;
			}

			cylinder->updateThing(item, itemId, count);
			cylinder->postAddNotification(item, cylinder, itemIndex);
			return item;
		}
	}

	//Replacing the old item with the new while maintaining the old position
	Item* newItem;
	if (newCount == -1) {
		newItem = Item::CreateItem(newId);
	} else {
		newItem = Item::CreateItem(newId, newCount);
	}

	if (newItem == nullptr) {
		return nullptr;
	}

	cylinder->replaceThing(itemIndex, newItem);
	cylinder->postAddNotification(newItem, cylinder, itemIndex);

	item->setParent(nullptr);
	cylinder->postRemoveNotification(item, cylinder, itemIndex);
	ReleaseItem(item);

	if (newItem->getDuration() > 0) {
		if (newItem->getDecaying() != DECAYING_TRUE) {
			newItem->incrementReferenceCounter();
			newItem->setDecaying(DECAYING_TRUE);
			toDecayItems.push_front(newItem);
		}
	}

	return newItem;
}

ReturnValue Game::internalTeleport(Thing* thing, const Position& newPos, bool pushMove/* = true*/, uint32_t flags /*= 0*/)
{
	if (newPos == thing->getPosition()) {
		return RETURNVALUE_NOERROR;
	} else if (thing->isRemoved()) {
		return RETURNVALUE_NOTPOSSIBLE;
	}

	Tile* toTile = map.getTile(newPos);
	if (!toTile || !toTile->getGround() && !toTile->getItemList()) {
		return RETURNVALUE_NOTPOSSIBLE;
	}

	if (Creature* creature = thing->getCreature()) {
		ReturnValue ret = toTile->queryAdd(0, *creature, 1, FLAG_NOLIMIT);
		if (ret != RETURNVALUE_NOERROR) {
			return ret;
		}

		map.moveCreature(*creature, *toTile, !pushMove);
		return RETURNVALUE_NOERROR;
	} else if (Item* item = thing->getItem()) {
		return internalMoveItem(item->getParent(), toTile, INDEX_WHEREEVER, item, item->getItemCount(), nullptr, flags);
	}
	return RETURNVALUE_NOTPOSSIBLE;
}

Item* searchForItem(Container* container, uint16_t itemId)
{
	for (ContainerIterator it = container->iterator(); it.hasNext(); it.advance()) {
		if ((*it)->getID() == itemId) {
			return *it;
		}
	}

	return nullptr;
}

slots_t getSlotType(const ItemType& it)
{
	slots_t slot = CONST_SLOT_RIGHT;
	if (it.weaponType != WeaponType_t::WEAPON_SHIELD) {
		int32_t slotPosition = it.slotPosition;

		if (slotPosition & SLOTP_HEAD) {
			slot = CONST_SLOT_HEAD;
		} else if (slotPosition & SLOTP_NECKLACE) {
			slot = CONST_SLOT_NECKLACE;
		} else if (slotPosition & SLOTP_ARMOR) {
			slot = CONST_SLOT_ARMOR;
		} else if (slotPosition & SLOTP_LEGS) {
			slot = CONST_SLOT_LEGS;
		} else if (slotPosition & SLOTP_FEET) {
			slot = CONST_SLOT_FEET;
		} else if (slotPosition & SLOTP_RING) {
			slot = CONST_SLOT_RING;
		} else if (slotPosition & SLOTP_AMMO) {
			slot = CONST_SLOT_AMMO;
		} else if (slotPosition & SLOTP_TWO_HAND || slotPosition & SLOTP_LEFT) {
			slot = CONST_SLOT_LEFT;
		}
	}

	return slot;
}

//Implementation of player invoked events
void Game::playerEquipItem(uint32_t playerId, uint16_t spriteId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Item* item = player->getInventoryItem(CONST_SLOT_BACKPACK);
	if (!item) {
		return;
	}

	Container* backpack = item->getContainer();
	if (!backpack) {
		return;
	}

	const ItemType& it = Item::items.getItemIdByClientId(spriteId);
	slots_t slot = getSlotType(it);

	Item* slotItem = player->getInventoryItem(slot);
	Item* equipItem = searchForItem(backpack, it.id);
	if (slotItem && slotItem->getID() == it.id && (!it.stackable || slotItem->getItemCount() == 100 || !equipItem)) {
		internalMoveItem(slotItem->getParent(), player, CONST_SLOT_WHEREEVER, slotItem, slotItem->getItemCount(), nullptr);
	} else if (equipItem) {
		internalMoveItem(equipItem->getParent(), player, slot, equipItem, equipItem->getItemCount(), nullptr);
	}
}

void Game::playerMove(uint32_t playerId, Direction direction)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	if (player->isMovementBlocked()) {
		player->clearToDo();
		player->sendCancelWalk();
		return;
	}

	player->resetIdleTime();

	if (player->clearToDo()) {
		player->sendCancelWalk();
	}

	player->addWalkToDo(direction);
	player->startToDo();
}

bool Game::playerBroadcastMessage(Player* player, const std::string& text) const
{
	if (!player->hasFlag(PlayerFlag_CanBroadcast)) {
		return false;
	}

	std::cout << "> " << player->getName() << " broadcasted: \"" << text << "\"." << std::endl;

	for (const auto& it : players) {
		it.second->sendPrivateMessage(player, TALKTYPE_BROADCAST, text);
	}

	return true;
}

void Game::playerCreatePrivateChannel(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if (!player || !player->isPremium()) {
		return;
	}

	ChatChannel* channel = g_chat->createChannel(*player, CHANNEL_PRIVATE);
	if (!channel || !channel->addUser(*player)) {
		return;
	}

	player->sendCreatePrivateChannel(channel->getId(), channel->getName());
}

void Game::playerChannelInvite(uint32_t playerId, const std::string& name)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	PrivateChatChannel* channel = g_chat->getPrivateChannel(*player);
	if (!channel) {
		return;
	}

	uint32_t targetGUID;
	std::string realPlayerName;

	Player* invitePlayer = getPlayerByName(name);
	if (invitePlayer) {
		targetGUID = invitePlayer->getGUID();
		realPlayerName = invitePlayer->getName();
	} else {
		targetGUID = IOLoginData::getGuidByName(name);
		realPlayerName = IOLoginData::getNameByGuid(targetGUID);
	}

	if (targetGUID == 0) {
		player->sendCancelMessage(RETURNVALUE_PLAYERDOESNOTEXIST);
		return;
	}

	if (targetGUID == player->getGUID()) {
		// Cannot invite self player to private channel.
		return;
	}

	if (channel->isInvited(targetGUID)) {
		player->sendTextMessage(MESSAGE_INFO_DESCR, fmt::format("{:s} has already been invited.", realPlayerName));
		return;
	}

	channel->invitePlayer(*player, targetGUID);

	player->sendTextMessage(MESSAGE_INFO_DESCR, fmt::format("{:s} has been invited.", realPlayerName));

	if (invitePlayer) {
		invitePlayer->sendTextMessage(MESSAGE_INFO_DESCR, fmt::format("{:s} invites you to {:s} private chat channel.", player->getName(), player->getSex() == PLAYERSEX_FEMALE ? "her" : "his"));
	}
}

void Game::playerChannelExclude(uint32_t playerId, const std::string& name)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	PrivateChatChannel* channel = g_chat->getPrivateChannel(*player);
	if (!channel) {
		return;
	}

	uint32_t targetGUID;
	std::string realName;

	Player* excludePlayer = getPlayerByName(name);
	if (excludePlayer) {
		targetGUID = excludePlayer->getGUID();
		realName = excludePlayer->getName();
	} else {
		targetGUID = IOLoginData::getGuidByName(name);
		realName = IOLoginData::getNameByGuid(targetGUID);
	}

	if (targetGUID == 0) {
		player->sendCancelMessage(RETURNVALUE_PLAYERDOESNOTEXIST);
		return;
	}

	if (targetGUID == player->getGUID()) {
		// Cannot exclude self from private channel.
		return;
	}

	if (!channel->isInvited(targetGUID)) {
		player->sendCancelMessage(fmt::format("{:s} has not been invited.", realName));
		return;
	}

	channel->excludePlayer(*player, targetGUID);

	player->sendTextMessage(MESSAGE_INFO_DESCR, fmt::format("{:s} has been excluded.", realName));
}

void Game::playerRequestChannels(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	player->sendChannelsDialog();
}

void Game::playerOpenChannel(uint32_t playerId, uint16_t channelId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	ChatChannel* channel = g_chat->addUserToChannel(*player, channelId);
	if (!channel) {
		return;
	}

	if (channel->getId() == CHANNEL_RULE_REP) {
		player->sendRuleViolationsChannel(channel->getId());
	} else {
		player->sendChannel(channel->getId(), channel->getName());
	}
}

void Game::playerCloseChannel(uint32_t playerId, uint16_t channelId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	g_chat->removeUserFromChannel(*player, channelId);
}

void Game::playerOpenPrivateChannel(uint32_t playerId, std::string& receiver)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	if (!IOLoginData::formatPlayerName(receiver)) {
		player->sendCancelMessage(RETURNVALUE_PLAYERDOESNOTEXIST);
		return;
	}

	if (player->getName() == receiver) {
		player->sendCancelMessage("You cannot set up a private message channel with yourself.");
		return;
	}

	player->sendOpenPrivateChannel(receiver);
}

void Game::playerReceivePing(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	player->receivePing();
}

void Game::playerReceivePingBack(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	player->sendPingBack();
}

void Game::playerAutoWalk(uint32_t playerId, const std::vector<Direction> listDir)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	player->resetIdleTime();

	player->addWalkToDo(listDir);
	player->startToDo();
}

void Game::playerStopAutoWalk(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	player->stopToDo();
}

void Game::playerUseItemEx(uint32_t playerId, const Position fromPos, uint8_t fromStackPos, uint16_t fromSpriteId,
                           Position toPos, uint8_t toStackPos, uint16_t toSpriteId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Thing* thing = internalGetThing(player, fromPos, fromStackPos, fromSpriteId, STACKPOS_USEITEM);
	if (!thing) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		return;
	}

	Item* item = thing->getItem();
	if (!item || !item->isUseable() || item->getClientID() != fromSpriteId) {
		player->sendCancelMessage(RETURNVALUE_CANNOTUSETHISOBJECT);
		return;
	}

	if (item->isRune() && toPos.x != 0xFFFF && player->getPosition().z != toPos.z) {
		player->sendCancelMessage(RETURNVALUE_DESTINATIONOUTOFREACH);
		return;
	}

	Position walkToPos = fromPos;
	ReturnValue ret = g_actions->canUse(player, fromPos);
	if (ret == RETURNVALUE_NOERROR) {
		ret = g_actions->canUse(player, toPos, item);
		if (ret == RETURNVALUE_TOOFARAWAY) {
			walkToPos = toPos;
		}
	}

	if (ret != RETURNVALUE_NOERROR) {
		if (ret == RETURNVALUE_TOOFARAWAY) {
			Position itemPos = fromPos;
			uint8_t itemStackPos = fromStackPos;

			if (fromPos.x != 0xFFFF && toPos.x != 0xFFFF && Position::areInRange<1, 1, 0>(fromPos, player->getPosition()) &&
			        (!Position::areInRange<1, 1, 0>(fromPos, toPos) || Item::items[item->getID()].isFluidContainer())) {

				if (!Position::areInRange<8, 8>(fromPos, toPos)) {
					if (item->isRune()) {
						player->sendCancelMessage(RETURNVALUE_DESTINATIONOUTOFREACH);
					} else {
						player->sendCancelMessage(RETURNVALUE_THEREISNOWAY);
					}
					return;
				}

				Item* moveItem = nullptr;

				ret = internalMoveItem(item->getParent(), player, INDEX_WHEREEVER, item, item->getItemCount(), &moveItem, 0, player, nullptr, &fromPos, &toPos);
				if (ret != RETURNVALUE_NOERROR) {
					player->sendCancelMessage(ret);
					return;
				}

				//changing the position since its now in the inventory of the player
				internalGetPosition(moveItem, itemPos, itemStackPos);
			}

			std::vector<Direction> listDir;
			if (player->getPathTo(walkToPos, listDir, 0, 1, true, false)) {
				player->addWalkToDo(listDir);
				player->addWaitToDo(g_config.getNumber(ConfigManager::ACTIONS_DELAY_INTERVAL));
				if (toSpriteId < 100 && g_config.getBoolean(ConfigManager::ALLOW_RUNEAIMBOT_FROM_DISTANCE)) {
					Tile* tile = map.getTile(toPos);
					Creature* topCreature = tile->getTopCreature();
					if (!topCreature) {
						std::cout << "Game::playerUseItemEx: topCreature is nullptr" << std::endl;
						return;
					}

					player->addActionToDo(TODO_USEEX,std::bind(&Game::playerUseWithCreature, this,
						playerId, itemPos, itemStackPos, topCreature->getID(), fromSpriteId));
				} else {
					player->addActionToDo(TODO_USEEX,std::bind(&Game::playerUseItemEx, this,
						playerId, itemPos, itemStackPos, fromSpriteId, toPos, toStackPos, toSpriteId));
				}
				player->startToDo();
			} else {
				player->sendCancelMessage(RETURNVALUE_THEREISNOWAY);
			}
			return;
		}

		player->sendCancelMessage(ret);
		return;
	}

	player->resetIdleTime();
	player->earliestMultiUseTime = OTSYS_TIME() + g_config.getNumber(ConfigManager::EX_ACTIONS_DELAY_INTERVAL);

	g_actions->useItemEx(player, fromPos, toPos, toStackPos, toSpriteId, item);
}

void Game::playerUseItem(uint32_t playerId, const Position pos, uint8_t stackPos,
                         uint8_t index, uint16_t spriteId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Thing* thing = internalGetThing(player, pos, stackPos, spriteId, STACKPOS_USEITEM);
	if (!thing) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		return;
	}

	Item* item = thing->getItem();
	if (!item || item->isUseable() || item->getClientID() != spriteId) {
		player->sendCancelMessage(RETURNVALUE_CANNOTUSETHISOBJECT);
		return;
	}

	ReturnValue ret = g_actions->canUse(player, pos);
	if (ret != RETURNVALUE_NOERROR) {
		if (ret == RETURNVALUE_TOOFARAWAY) {
			std::vector<Direction> listDir;
			if (player->getPathTo(pos, listDir, 0, 1, true, false, 12)) {
				player->addWalkToDo(listDir);
				player->addWaitToDo(g_config.getNumber(ConfigManager::ACTIONS_DELAY_INTERVAL));
				player->addActionToDo(std::bind(&Game::playerUseItem, this,
					playerId, pos, stackPos, index, spriteId));
				player->startToDo();
				return;
			}

			ret = RETURNVALUE_THEREISNOWAY;
		}

		player->sendCancelMessage(ret);
		return;
	}

	player->resetIdleTime();

	g_actions->useItem(player, pos, index, item);
}

void Game::playerUseWithCreature(uint32_t playerId, const Position fromPos, uint8_t fromStackPos, uint32_t creatureId, uint16_t spriteId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Creature* creature = getCreatureByID(creatureId);
	if (!creature) {
		return;
	}

	Thing* thing = internalGetThing(player, fromPos, fromStackPos, spriteId, STACKPOS_USEITEM);
	if (!thing) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		return;
	}

	Item* item = thing->getItem();
	if (!item || !item->isUseable() || item->getClientID() != spriteId) {
		player->sendCancelMessage(RETURNVALUE_CANNOTUSETHISOBJECT);
		return;
	}

	if (!Position::areInRange<7, 6, 0>(creature->getPosition(), player->getPosition())) {
		if (item->isRune()) {
			player->sendCancelMessage(RETURNVALUE_DESTINATIONOUTOFREACH);
		} else {
			player->sendCancelMessage(RETURNVALUE_THEREISNOWAY);
		}
		return;
	}

	Position toPos = creature->getPosition();
	Position walkToPos = fromPos;
	ReturnValue ret = g_actions->canUse(player, fromPos);
	if (ret == RETURNVALUE_NOERROR) {
		ret = g_actions->canUse(player, toPos, item);
		if (ret == RETURNVALUE_TOOFARAWAY) {
			walkToPos = toPos;
		}
	}

	if (ret != RETURNVALUE_NOERROR) {
		if (ret == RETURNVALUE_TOOFARAWAY) {
			Position itemPos = fromPos;
			uint8_t itemStackPos = fromStackPos;

			if (fromPos.x != 0xFFFF && Position::areInRange<1, 1, 0>(fromPos, player->getPosition()) && !Position::areInRange<1, 1, 0>(fromPos, toPos)) {
				Item* moveItem = nullptr;
				ret = internalMoveItem(item->getParent(), player, INDEX_WHEREEVER, item, item->getItemCount(), &moveItem, 0, player, nullptr, &fromPos, &toPos);
				if (ret != RETURNVALUE_NOERROR) {
					player->sendCancelMessage(ret);
					return;
				}

				//changing the position since its now in the inventory of the player
				internalGetPosition(moveItem, itemPos, itemStackPos);
			}

			std::vector<Direction> listDir;
			if (player->getPathTo(walkToPos, listDir, 0, 1, true, true)) {
				player->addWalkToDo(listDir);
				player->addActionToDo(std::bind(&Game::playerUseWithCreature, this,
					playerId, itemPos, itemStackPos, creatureId, spriteId));
				player->startToDo();
			} else {
				player->sendCancelMessage(RETURNVALUE_THEREISNOWAY);
			}
			return;
		}

		player->sendCancelMessage(ret);
		return;
	}

	player->resetIdleTime();
	player->earliestMultiUseTime = OTSYS_TIME() + g_config.getNumber(ConfigManager::EX_ACTIONS_DELAY_INTERVAL);

	g_actions->useItemEx(player, fromPos, creature->getPosition(), creature->getParent()->getThingIndex(creature), 99, item, creature);
}

void Game::playerCloseContainer(uint32_t playerId, uint8_t cid)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	player->closeContainer(cid);
	player->sendCloseContainer(cid);
}

void Game::playerMoveUpContainer(uint32_t playerId, uint8_t cid)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Container* container = player->getContainerByID(cid);
	if (!container) {
		return;
	}

	Container* parentContainer = dynamic_cast<Container*>(container->getRealParent());
	if (!parentContainer) {
		return;
	}

	player->addContainer(cid, parentContainer);
	player->sendContainer(cid, parentContainer, parentContainer->hasParent());
}

void Game::playerUpdateContainer(uint32_t playerId, uint8_t cid)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Container* container = player->getContainerByID(cid);
	if (!container) {
		return;
	}

	player->sendContainer(cid, container, container->hasParent());
}

void Game::playerRotateItem(uint32_t playerId, const Position pos, uint8_t stackPos, const uint16_t spriteId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Thing* thing = internalGetThing(player, pos, stackPos, 0, STACKPOS_TOPDOWN_ITEM);
	if (!thing) {
		return;
	}

	Item* item = thing->getItem();
	if (!item || item->getClientID() != spriteId || !item->isRotatable() || item->hasAttribute(ITEM_ATTRIBUTE_UNIQUEID) || item->getActionId() >= 1000 && item->getActionId() <= 2000) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		return;
	}

	if (pos.x != 0xFFFF && !Position::areInRange<1, 1, 0>(pos, player->getPosition())) {
		std::vector<Direction> listDir;
		if (player->getPathTo(pos, listDir, 0, 1, true, true)) {
			player->addWalkToDo(listDir);
			player->addActionToDo(std::bind(&Game::playerRotateItem, this,
				playerId, pos, stackPos, spriteId));
			player->startToDo();
		} else {
			player->sendCancelMessage(RETURNVALUE_THEREISNOWAY);
		}
		return;
	}

	uint16_t newId = Item::items[item->getID()].rotateTo;
	if (newId != 0) {
		transformItem(item, newId);
	}
}

void Game::playerWriteItem(uint32_t playerId, uint32_t windowTextId, const std::string& text)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	uint16_t maxTextLength = 0;
	uint32_t internalWindowTextId = 0;

	Item* writeItem = player->getWriteItem(internalWindowTextId, maxTextLength);
	if (text.length() > maxTextLength || windowTextId != internalWindowTextId) {
		player->setWriteItem(nullptr);
		return;
	}

	if (!isASCII(text)) {
		player->setWriteItem(nullptr);
		return;
	}

	if (!writeItem || writeItem->isRemoved()) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		if (writeItem) {
			player->setWriteItem(nullptr);
		}
		return;
	}

	Cylinder* topParent = writeItem->getTopParent();

	Player* owner = dynamic_cast<Player*>(topParent);
	if (owner && owner != player) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		player->setWriteItem(nullptr);
		return;
	}

	if (!Position::areInRange<1, 1, 0>(writeItem->getPosition(), player->getPosition())) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		player->setWriteItem(nullptr);
		return;
	}

	for (auto creatureEvent : player->getCreatureEvents(CREATURE_EVENT_TEXTEDIT)) {
		if (!creatureEvent->executeTextEdit(player, writeItem, text)) {
			player->setWriteItem(nullptr);
			return;
		}
	}

	if (!text.empty()) {
		if (writeItem->getText() != text) {
			writeItem->setText(text);
			writeItem->setWriter(player->getName());
			writeItem->setDate(time(nullptr));
		}
	} else {
		writeItem->resetText();
		writeItem->resetWriter();
		writeItem->resetDate();
	}

	uint16_t newId = Item::items[writeItem->getID()].writeOnceItemId;
	if (newId != 0) {
		transformItem(writeItem, newId);
	}

	player->setWriteItem(nullptr);
}

void Game::playerUpdateHouseWindow(uint32_t playerId, uint8_t listId, uint32_t windowTextId, const std::string& text)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	uint32_t internalWindowTextId;
	uint32_t internalListId;

	House* house = player->getEditHouse(internalWindowTextId, internalListId);
	if (house && house->canEditAccessList(internalListId, player) && internalWindowTextId == windowTextId && listId == 0) {
		if (isASCII(text)) {
			StringVector lines = explodeString(text, "\n");
			if (lines.empty()) {
				lines.push_back(text);
			}
			std::stringstream ss;
			for (auto& line : lines) {
				if (!line.starts_with('#')) {
					ss << line;
					if (!line.ends_with('\n')) {
						ss << std::endl;
					}
				}
			}
			house->setAccessList(internalListId, ss.str());
		}
	}

	player->setEditHouse(nullptr);
}

void Game::playerRequestTrade(uint32_t playerId, const Position pos, uint8_t stackPos,
                              uint32_t tradePlayerId, uint16_t spriteId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Player* tradePartner = getPlayerByID(tradePlayerId);
	if (!tradePartner || tradePartner == player) {
		player->sendCancelMessage("Select a player to trade with.");
		return;
	}

	if (!Position::areInRange<2, 2, 0>(tradePartner->getPosition(), player->getPosition())) {
		player->sendCancelMessage(RETURNVALUE_DESTINATIONOUTOFREACH);
		return;
	}

	if (!canThrowObjectTo(tradePartner->getPosition(), player->getPosition(), false)) {
		player->sendCancelMessage(RETURNVALUE_CANNOTTHROW);
		return;
	}

	Thing* tradeThing = internalGetThing(player, pos, stackPos, 0, STACKPOS_TOPDOWN_ITEM);
	if (!tradeThing) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		return;
	}

	Item* tradeItem = tradeThing->getItem();
	if (tradeItem->getClientID() != spriteId || !tradeItem->isPickupable() || tradeItem->hasAttribute(ITEM_ATTRIBUTE_UNIQUEID) || tradeItem->getActionId() >= 1000 && tradeItem->getActionId() <= 2000) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		return;
	}

	if (g_config.getBoolean(ConfigManager::ONLY_INVITED_CAN_MOVE_HOUSE_ITEMS)) {
		if (const House* house = tradeItem->getTile()->getHouse()) {
			if (!tradeItem->getTopParent()->getCreature() && !house->isInvited(player)) {
				player->sendCancelMessage(RETURNVALUE_PLAYERISNOTINVITED);
				return;
			}
		}
	}

	const Position& playerPosition = player->getPosition();
	const Position& tradeItemPosition = tradeItem->getPosition();
	if (playerPosition.z != tradeItemPosition.z) {
		player->sendCancelMessage(playerPosition.z > tradeItemPosition.z ? RETURNVALUE_FIRSTGOUPSTAIRS : RETURNVALUE_FIRSTGODOWNSTAIRS);
		return;
	}

	if (!Position::areInRange<1, 1>(tradeItemPosition, playerPosition)) {
		std::vector<Direction> listDir;
		if (player->getPathTo(pos, listDir, 0, 1, true, true)) {
			player->addWalkToDo(listDir);
			player->addActionToDo(std::bind(&Game::playerRequestTrade, this,
				playerId, pos, stackPos, tradePlayerId, spriteId));
			player->startToDo();
		} else {
			player->sendCancelMessage(RETURNVALUE_THEREISNOWAY);
		}
		return;
	}

	if (player->tradeItem) {
		player->sendCancelMessage("You are already trading. Finish this trade first.");
		return;
	}

	Container* tradeItemContainer = tradeItem->getContainer();
	if (tradeItemContainer) {
		for (const auto& it : tradeItems) {
			Item* item = it.first;
			if (tradeItem == item) {
				player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
				return;
			}

			if (tradeItemContainer->isHoldingItem(item)) {
				player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
				return;
			}

			Container* container = item->getContainer();
			if (container && container->isHoldingItem(tradeItem)) {
				player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
				return;
			}
		}
	} else {
		for (const auto& it : tradeItems) {
			Item* item = it.first;
			if (tradeItem == item) {
				player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
				return;
			}

			Container* container = item->getContainer();
			if (container && container->isHoldingItem(tradeItem)) {
				player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
				return;
			}
		}
	}

	Container* tradeContainer = tradeItem->getContainer();
	if (tradeContainer && tradeContainer->getItemHoldingCount() + 1 > 100) {
		player->sendCancelMessage("You can only trade up to 100 objects at once.");
		return;
	}

	if (!g_events->eventPlayerOnTradeRequest(player, tradePartner, tradeItem)) {
		return;
	}

	internalStartTrade(player, tradePartner, tradeItem);
}

bool Game::internalStartTrade(Player* player, Player* tradePartner, Item* tradeItem)
{
	if (player->tradeState != TRADE_NONE && !(player->tradeState == TRADE_ACKNOWLEDGE && player->tradePartner == tradePartner)) {
		player->sendCancelMessage(RETURNVALUE_YOUAREALREADYTRADING);
		return false;
	} else if (tradePartner->tradeState != TRADE_NONE && tradePartner->tradePartner != player) {
		player->sendCancelMessage(RETURNVALUE_THISPLAYERISALREADYTRADING);
		return false;
	}

	player->tradePartner = tradePartner;
	player->tradeItem = tradeItem;
	player->tradeState = TRADE_INITIATED;
	tradeItem->incrementReferenceCounter();
	tradeItems[tradeItem] = player->getID();

	player->sendTradeItemRequest(player->getName(), tradeItem, true);

	if (tradePartner->tradeState == TRADE_NONE) {
		tradePartner->sendTextMessage(MESSAGE_INFO_DESCR, fmt::format("{:s} wants to trade with you.", player->getName()));
		tradePartner->tradeState = TRADE_ACKNOWLEDGE;
		tradePartner->tradePartner = player;
	} else {
		Item* counterOfferItem = tradePartner->tradeItem;
		player->sendTradeItemRequest(tradePartner->getName(), counterOfferItem, false);
		tradePartner->sendTradeItemRequest(player->getName(), tradeItem, false);
	}

	return true;
}

void Game::playerAcceptTrade(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	if (!(player->getTradeState() == TRADE_ACKNOWLEDGE || player->getTradeState() == TRADE_INITIATED)) {
		return;
	}

	Player* tradePartner = player->tradePartner;
	if (!tradePartner) {
		return;
	}

	player->setTradeState(TRADE_ACCEPT);

	if (tradePartner->getTradeState() == TRADE_ACCEPT) {
		if (!canThrowObjectTo(tradePartner->getPosition(), player->getPosition(), false)) {
			internalCloseTrade(player, false);
			player->sendCancelMessage(RETURNVALUE_CANNOTTHROW);
			tradePartner->sendCancelMessage(RETURNVALUE_CANNOTTHROW);
			return;
		}

		Item* playerTradeItem = player->tradeItem;
		Item* partnerTradeItem = tradePartner->tradeItem;

		if (!g_events->eventPlayerOnTradeAccept(player, tradePartner, playerTradeItem, partnerTradeItem)) {
			internalCloseTrade(player, false);
			return;
		}

		player->setTradeState(TRADE_TRANSFER);
		tradePartner->setTradeState(TRADE_TRANSFER);

		auto it = tradeItems.find(playerTradeItem);
		if (it != tradeItems.end()) {
			ReleaseItem(it->first);
			tradeItems.erase(it);
		}

		it = tradeItems.find(partnerTradeItem);
		if (it != tradeItems.end()) {
			ReleaseItem(it->first);
			tradeItems.erase(it);
		}

		bool isSuccess = false;

		ReturnValue tradePartnerRet = RETURNVALUE_NOERROR;
		ReturnValue playerRet = RETURNVALUE_NOERROR;

		// if player is trying to trade its own backpack
		if (tradePartner->getInventoryItem(CONST_SLOT_BACKPACK) == partnerTradeItem) {
			tradePartnerRet = (tradePartner->getInventoryItem(getSlotType(Item::items[playerTradeItem->getID()])) ? RETURNVALUE_NOTENOUGHROOM : RETURNVALUE_NOERROR);
		}

		if (player->getInventoryItem(CONST_SLOT_BACKPACK) == playerTradeItem) {
			playerRet = (player->getInventoryItem(getSlotType(Item::items[partnerTradeItem->getID()])) ? RETURNVALUE_NOTENOUGHROOM : RETURNVALUE_NOERROR);
		}

		// both players try to trade equipped backpacks
		if (player->getInventoryItem(CONST_SLOT_BACKPACK) == playerTradeItem && tradePartner->getInventoryItem(CONST_SLOT_BACKPACK) == partnerTradeItem) {
			playerRet = RETURNVALUE_NOTENOUGHROOM;
		}

		if (tradePartnerRet == RETURNVALUE_NOERROR && playerRet == RETURNVALUE_NOERROR) {
			tradePartnerRet = internalAddItem(tradePartner, playerTradeItem, INDEX_WHEREEVER, FLAG_IGNOREAUTOSTACK, true);
			playerRet = internalAddItem(player, partnerTradeItem, INDEX_WHEREEVER, FLAG_IGNOREAUTOSTACK, true);
			if (tradePartnerRet == RETURNVALUE_NOERROR && playerRet == RETURNVALUE_NOERROR) {
				playerRet = internalRemoveItem(playerTradeItem, playerTradeItem->getItemCount(), true);
				tradePartnerRet = internalRemoveItem(partnerTradeItem, partnerTradeItem->getItemCount(), true);
				if (tradePartnerRet == RETURNVALUE_NOERROR && playerRet == RETURNVALUE_NOERROR) {
					tradePartnerRet = internalMoveItem(playerTradeItem->getParent(), tradePartner, INDEX_WHEREEVER, playerTradeItem, playerTradeItem->getItemCount(), nullptr, FLAG_IGNOREAUTOSTACK, nullptr, partnerTradeItem);
					if (tradePartnerRet == RETURNVALUE_NOERROR) {
						internalMoveItem(partnerTradeItem->getParent(), player, INDEX_WHEREEVER, partnerTradeItem, partnerTradeItem->getItemCount(), nullptr, FLAG_IGNOREAUTOSTACK);
						playerTradeItem->onTradeEvent(ON_TRADE_TRANSFER, tradePartner);
						partnerTradeItem->onTradeEvent(ON_TRADE_TRANSFER, player);
						isSuccess = true;
					}
				}
			}
		}

		if (!isSuccess) {
			std::string errorDescription;

			if (tradePartner->tradeItem) {
				errorDescription = getTradeErrorDescription(tradePartnerRet, playerTradeItem);
				tradePartner->sendCancelMessage(errorDescription);
				tradePartner->tradeItem->onTradeEvent(ON_TRADE_CANCEL, tradePartner);
			}

			if (player->tradeItem) {
				player->tradeItem->onTradeEvent(ON_TRADE_CANCEL, player);
			}
		}

		g_events->eventPlayerOnTradeCompleted(player, tradePartner, playerTradeItem, partnerTradeItem, isSuccess);

		player->setTradeState(TRADE_NONE);
		player->tradeItem = nullptr;
		player->tradePartner = nullptr;
		player->sendTradeClose();

		tradePartner->setTradeState(TRADE_NONE);
		tradePartner->tradeItem = nullptr;
		tradePartner->tradePartner = nullptr;
		tradePartner->sendTradeClose();
	}
}

std::string Game::getTradeErrorDescription(ReturnValue ret, Item* item)
{
	if (item) {
		if (ret == RETURNVALUE_NOTENOUGHCAPACITY) {
			return "This object is too heavy.";
		}
	}
	return "There is not enough room.";
}

void Game::playerLookInTrade(uint32_t playerId, bool lookAtCounterOffer, uint8_t index)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Player* tradePartner = player->tradePartner;
	if (!tradePartner) {
		return;
	}

	Item* tradeItem;
	if (lookAtCounterOffer) {
		tradeItem = tradePartner->getTradeItem();
	} else {
		tradeItem = player->getTradeItem();
	}

	if (!tradeItem) {
		return;
	}

	const Position& playerPosition = player->getPosition();
	const Position& tradeItemPosition = tradeItem->getPosition();

	int32_t lookDistance = std::max<int32_t>(Position::getDistanceX(playerPosition, tradeItemPosition),
	                                         Position::getDistanceY(playerPosition, tradeItemPosition));
	if (index == 0) {
		g_events->eventPlayerOnLookInTrade(player, tradePartner, tradeItem, lookDistance);
		return;
	}

	Container* tradeContainer = tradeItem->getContainer();
	if (!tradeContainer) {
		return;
	}

	std::vector<const Container*> containers {tradeContainer};
	size_t i = 0;
	while (i < containers.size()) {
		const Container* container = containers[i++];
		for (Item* item : container->getItemList()) {
			Container* tmpContainer = item->getContainer();
			if (tmpContainer) {
				containers.push_back(tmpContainer);
			}

			if (--index == 0) {
				g_events->eventPlayerOnLookInTrade(player, tradePartner, item, lookDistance);
				return;
			}
		}
	}
}

void Game::playerCloseTrade(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	internalCloseTrade(player);
}

void Game::internalCloseTrade(Player* player, bool sendCancel/* = true*/)
{
	Player* tradePartner = player->tradePartner;
	if ((tradePartner && tradePartner->getTradeState() == TRADE_TRANSFER) || player->getTradeState() == TRADE_TRANSFER) {
		return;
	}

	if (player->getTradeItem()) {
		auto it = tradeItems.find(player->getTradeItem());
		if (it != tradeItems.end()) {
			ReleaseItem(it->first);
			tradeItems.erase(it);
		}

		player->tradeItem->onTradeEvent(ON_TRADE_CANCEL, player);
		player->tradeItem = nullptr;
	}

	player->setTradeState(TRADE_NONE);
	player->tradePartner = nullptr;

	if (sendCancel) {
		player->sendTextMessage(MESSAGE_STATUS_SMALL, "Trade cancelled.");
	}
	player->sendTradeClose();

	if (tradePartner) {
		if (tradePartner->getTradeItem()) {
			auto it = tradeItems.find(tradePartner->getTradeItem());
			if (it != tradeItems.end()) {
				ReleaseItem(it->first);
				tradeItems.erase(it);
			}

			tradePartner->tradeItem->onTradeEvent(ON_TRADE_CANCEL, tradePartner);
			tradePartner->tradeItem = nullptr;
		}

		tradePartner->setTradeState(TRADE_NONE);
		tradePartner->tradePartner = nullptr;

		if (sendCancel) {
			tradePartner->sendTextMessage(MESSAGE_STATUS_SMALL, "Trade cancelled.");
		}
		tradePartner->sendTradeClose();
	}
}

void Game::playerLookAt(uint32_t playerId, const Position& pos, uint8_t stackPos)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Thing* thing = internalGetThing(player, pos, stackPos, 0, STACKPOS_LOOK);
	if (!thing) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		return;
	}

	Position thingPos = thing->getPosition();
	if (!player->canSee(thingPos)) {
		player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
		return;
	}

	Position playerPos = player->getPosition();

	int32_t lookDistance;
	if (thing != player) {
		lookDistance = std::max<int32_t>(Position::getDistanceX(playerPos, thingPos), Position::getDistanceY(playerPos, thingPos));
		if (playerPos.z != thingPos.z) {
			lookDistance += 15;
		}
	} else {
		lookDistance = -1;
	}

	g_events->eventPlayerOnLook(player, pos, thing, stackPos, lookDistance);
}

void Game::playerLookInBattleList(uint32_t playerId, uint32_t creatureId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Creature* creature = getCreatureByID(creatureId);
	if (!creature) {
		return;
	}

	if (!player->canSeeCreature(creature)) {
		return;
	}

	const Position& creaturePos = creature->getPosition();
	if (!player->canSee(creaturePos)) {
		return;
	}

	int32_t lookDistance;
	if (creature != player) {
		const Position& playerPos = player->getPosition();
		lookDistance = std::max<int32_t>(Position::getDistanceX(playerPos, creaturePos), Position::getDistanceY(playerPos, creaturePos));
		if (playerPos.z != creaturePos.z) {
			lookDistance += 15;
		}
	} else {
		lookDistance = -1;
	}

	g_events->eventPlayerOnLookInBattleList(player, creature, lookDistance);
}

void Game::playerCancelAttackAndFollow(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	playerSetAttackedCreature(playerId, 0);
	playerFollowCreature(playerId, 0);

	player->stopToDo();
}

void Game::playerSetAttackedCreature(uint32_t playerId, uint32_t creatureId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	if (player->getAttackedCreature() && creatureId == 0) {
		player->setAttackedCreature(nullptr);
		player->sendCancelTarget();
		return;
	}

	Creature* attackCreature = getCreatureByID(creatureId);
	if (!attackCreature) {
		player->setAttackedCreature(nullptr);
		player->sendCancelTarget();
		return;
	}

	if (attackCreature && !Position::areInRange<8, 8>(player->getPosition(), attackCreature->getPosition())) {
		player->sendCancelTarget();
		return;
	}

	ReturnValue ret = Combat::canTargetCreature(player, attackCreature);
	if (ret != RETURNVALUE_NOERROR) {
		player->sendCancelMessage(ret);
		player->sendCancelTarget();
		player->setAttackedCreature(nullptr);
		return;
	}

	player->setFollowCreature(nullptr);
	player->setAttackedCreature(attackCreature);
	player->addYieldToDo();
}

void Game::playerFollowCreature(uint32_t playerId, uint32_t creatureId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Creature* target = getCreatureByID(creatureId);
	if (target && !Position::areInRange<8, 8>(player->getPosition(), target->getPosition())) {
		player->sendCancelTarget();
		return;
	}

	player->setAttackedCreature(nullptr);
	player->setFollowCreature(getCreatureByID(creatureId));
}

void Game::playerSetFightModes(uint32_t playerId, fightMode_t fightMode, bool chaseMode, bool secureMode)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	if (player->fightMode != fightMode) {
		// Delay attacks when changing attacking stance
		player->earliestAttackTime = OTSYS_TIME() + player->getAttackSpeed();
	}

	player->setFightMode(fightMode);
	player->setChaseMode(chaseMode);
	player->setSecureMode(secureMode);
	player->targetClearRound = 0;
}

void Game::playerRequestAddVip(uint32_t playerId, const std::string& name)
{
	if (name.length() >= PLAYER_NAME_MAXLENGTH) {
		return;
	}

	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Player* vipPlayer = getPlayerByName(name);
	if (!vipPlayer) {
		uint32_t guid;
		bool specialVip;
		std::string formattedName = name;
		if (!IOLoginData::getGuidByNameEx(guid, specialVip, formattedName)) {
			player->sendCancelMessage(RETURNVALUE_PLAYERDOESNOTEXIST);
			return;
		}

		if (specialVip && !player->hasFlag(PlayerFlag_SpecialVIP)) {
			player->sendTextMessage(MESSAGE_STATUS_SMALL, "You can not add this player.");
			return;
		}

		player->addVIP(guid, formattedName, VIPSTATUS_OFFLINE);
	} else {
		if (vipPlayer->hasFlag(PlayerFlag_SpecialVIP) && !player->hasFlag(PlayerFlag_SpecialVIP)) {
			player->sendTextMessage(MESSAGE_STATUS_SMALL, "You can not add this player.");
			return;
		}

		if (!vipPlayer->isInGhostMode() || player->canSeeGhostMode(vipPlayer)) {
			player->addVIP(vipPlayer->getGUID(), vipPlayer->getName(), VIPSTATUS_ONLINE);
		} else {
			player->addVIP(vipPlayer->getGUID(), vipPlayer->getName(), VIPSTATUS_OFFLINE);
		}
	}
}

void Game::playerRequestRemoveVip(uint32_t playerId, uint32_t guid)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	player->removeVIP(guid);
}

void Game::playerTurn(uint32_t playerId, Direction dir)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	if (!g_events->eventPlayerOnTurn(player, dir)) {
		return;
	}

	player->resetIdleTime();
	
	if (player->isExecuting && player->clearToDo()) {
		player->sendCancelWalk();
	}

	player->addActionToDo(std::bind(&Game::creatureTurn, this, player, dir));
	player->startToDo();
}

void Game::playerRequestOutfit(uint32_t playerId)
{
	if (!g_config.getBoolean(ConfigManager::ALLOW_CHANGEOUTFIT)) {
		return;
	}

	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	player->sendOutfitWindow();
}

void Game::playerChangeOutfit(uint32_t playerId, Outfit_t outfit)
{
	if (!g_config.getBoolean(ConfigManager::ALLOW_CHANGEOUTFIT)) {
		return;
	}

	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	if (player->canWear(outfit.lookType)) {
		player->defaultOutfit = outfit;

		if (player->hasCondition(CONDITION_OUTFIT)) {
			return;
		}

		internalCreatureChangeOutfit(player, outfit);
	}
}

void Game::playerSay(uint32_t playerId, uint16_t channelId, SpeakClasses type,
	const std::string& receiver, std::string text)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	player->resetIdleTime();

	int32_t Muting = player->isMuted();
	if (Muting > 0) {
		player->sendTextMessage(MESSAGE_STATUS_SMALL, fmt::format("You are still muted for {:d} second{:s}.", Muting, Muting == 1 ? "" : "s"));
		return;
	}

	Position pos = player->getPosition();
	if (playerSaySpell(player, type, text)) {
		return;
	}

	Muting = player->addMessageBuffer();
	if (Muting > 0) {
		player->sendTextMessage(MESSAGE_STATUS_SMALL, fmt::format("You are muted for {:d} second{:s}.", Muting, Muting == 1 ? "" : "s"));
		return;
	}

	if (!text.empty() && text.front() == '/' && player->isAccessPlayer()) {
		return;
	}

	switch (type) {
		case TALKTYPE_SAY:
			internalCreatureSay(player, TALKTYPE_SAY, text, false, nullptr, &pos);
			break;

		case TALKTYPE_WHISPER:
			playerWhisper(player, text);
			break;

		case TALKTYPE_YELL:
			playerYell(player, text);
			break;

		case TALKTYPE_PRIVATE:
		case TALKTYPE_PRIVATE_RED:
		case TALKTYPE_RVR_ANSWER:
			playerSpeakTo(player, type, receiver, text);
			break;

		case TALKTYPE_CHANNEL_O:
		case TALKTYPE_CHANNEL_Y:
		case TALKTYPE_CHANNEL_R1:
		case TALKTYPE_CHANNEL_R2: {
			if (channelId == CHANNEL_RULE_REP) {
				internalCreatureSay(player, TALKTYPE_SAY, text, false, nullptr, &pos);
			} else {
				g_chat->talkToChannel(*player, type, text, channelId);
			}
			break;
		}

		case TALKTYPE_BROADCAST:
			playerBroadcastMessage(player, text);
			break;

		case TALKTYPE_RVR_CHANNEL:
			playerReportRuleViolationReport(player, text);
			break;

		case TALKTYPE_RVR_CONTINUE:
			playerContinueRuleViolationReport(player, text);
			break;

		default:
			break;
	}
}

void Game::registerFailedAccountLogin(uint32_t accountNumber)
{
	std::lock_guard<std::mutex> lock(gameLock);

	if (g_config.getNumber(ConfigManager::FAILED_LOGINATTEMPTS_ACCOUNT_LOCK) == 0) {
		return;
	}

	auto it = accountLoginAttemptsMap.find(accountNumber);
	if (it == accountLoginAttemptsMap.end()) {
		auto pair = std::make_pair<uint32_t, uint64_t>(1, OTSYS_TIME());
		accountLoginAttemptsMap[accountNumber] = pair;
	}
	else {
		it->second.first++;
		if (it->second.first >= g_config.getNumber(ConfigManager::FAILED_LOGINATTEMPTS_ACCOUNT_LOCK)) {
			it->second.second = OTSYS_TIME() + g_config.getNumber(ConfigManager::ACCOUNT_LOCK_DURATION);
		}
	}
}

bool Game::isAccountLocked(uint32_t accountNumber)
{
	std::lock_guard<std::mutex> lock(gameLock);

	auto it = accountLoginAttemptsMap.find(accountNumber);
	if (it == accountLoginAttemptsMap.end()) {
		return false;
	}

	bool locked = it->second.first >= g_config.getNumber(ConfigManager::FAILED_LOGINATTEMPTS_ACCOUNT_LOCK) && OTSYS_TIME() <= it->second.second;
	
	if (it->second.first >= g_config.getNumber(ConfigManager::FAILED_LOGINATTEMPTS_ACCOUNT_LOCK) && OTSYS_TIME() >= it->second.second) {
		it->second.first = 0;
		locked = false;
	}

	return locked;
}

void Game::resetIpLoginAttempts(uint32_t ip)
{
	std::lock_guard<std::mutex> lock(gameLock);

	auto it = ipLoginAttemptsMap.find(ip);
	if (it == ipLoginAttemptsMap.end()) {
		return;
	}

	if (OTSYS_TIME() - (g_config.getNumber(ConfigManager::IP_LOCK_DURATION)) >= it->second.second) {
		ipLoginAttemptsMap.erase(it);
	}
}

void Game::resetAccountLoginAttempts(uint32_t accountNumber)
{
	std::lock_guard<std::mutex> lock(gameLock);

	auto it = accountLoginAttemptsMap.find(accountNumber);
	if (it == accountLoginAttemptsMap.end()) {
		return;
	}

	accountLoginAttemptsMap.erase(it);
}

void Game::registerFailedIPLogin(uint32_t ip)
{
	std::lock_guard<std::mutex> lock(gameLock);

	if (g_config.getNumber(ConfigManager::FAILED_LOGINATTEMPTS_IP_BAN) == 0) {
		return;
	}

	auto it = ipLoginAttemptsMap.find(ip);
	if (it == ipLoginAttemptsMap.end()) {
		auto pair = std::make_pair<uint32_t, uint64_t>(1, OTSYS_TIME());
		ipLoginAttemptsMap[ip] = pair;
	}
	else {
		it->second.first++;
		if (it->second.first >= g_config.getNumber(ConfigManager::FAILED_LOGINATTEMPTS_IP_BAN)) {
			it->second.second = OTSYS_TIME() + g_config.getNumber(ConfigManager::IP_LOCK_DURATION);
		}
	}
}

bool Game::isIPLocked(uint32_t ip)
{
	std::lock_guard<std::mutex> lock(gameLock);

	auto it = ipLoginAttemptsMap.find(ip);
	if (it == ipLoginAttemptsMap.end()) {
		return false;
	}

	bool locked = it->second.first >= g_config.getNumber(ConfigManager::FAILED_LOGINATTEMPTS_IP_BAN) && OTSYS_TIME() <= it->second.second;
	
	if (it->second.first >= g_config.getNumber(ConfigManager::FAILED_LOGINATTEMPTS_IP_BAN) && OTSYS_TIME() >= it->second.second) {
		it->second.first = 0;
		locked = false;
	}

	return locked;
}

uint32_t Game::recordStatement(uint32_t guid, uint32_t mode, uint32_t channel, const std::string& text)
{
	static uint32_t statementId = 0;

	Statement statement;
	statement.characterID = guid;
	statement.mode = mode;
	statement.channel = channel;
	statement.text = text;
	statement.timeStamp = std::time(nullptr);

	statements.push_back(statement);
	return statementId - 1;
}

void Game::recordListener(uint32_t statementId, uint32_t guid)
{
	Listener listener;
	listener.StatementID = statementId;
	listener.CharacterID = guid;

	listeners.push_back(listener);
}

bool Game::playerSaySpell(Player* player, SpeakClasses type, const std::string& text)
{
	std::string words = text;

	TalkActionResult_t result = g_talkActions->playerSaySpell(player, type, words);
	if (result == TALKACTION_BREAK) {
		return true;
	}

	Position pos = player->getPosition();
	result = g_spells->playerSaySpell(player, words);
	if (result == TALKACTION_BREAK) {
		if (!g_config.getBoolean(ConfigManager::EMOTE_SPELLS)) {
			return internalCreatureSay(player, TALKTYPE_SAY, words, false, nullptr, &pos);
		} else {
			return internalCreatureSay(player, TALKTYPE_MONSTER_SAY, words, false, nullptr, &pos);
		}

	} else if (result == TALKACTION_FAILED) {
		return true;
	}

	return false;
}

void Game::playerWhisper(Player* player, const std::string& text)
{
	SpectatorVec spectators;
	map.getSpectators(spectators, player->getPosition(), false, false,
	              Map::maxClientViewportX, Map::maxClientViewportX,
	              Map::maxClientViewportY, Map::maxClientViewportY);

	uint32_t statementId = recordStatement(player->getGUID(), TALKTYPE_WHISPER, 0, text);

	//send to client
	for (Creature* spectator : spectators) {
		if (Player* spectatorPlayer = spectator->getPlayer()) {
			if (!Position::areInRange<1, 1>(player->getPosition(), spectatorPlayer->getPosition())) {
				spectatorPlayer->sendCreatureSay(0, player, TALKTYPE_WHISPER, "pspsps");
			} else {
				spectatorPlayer->sendCreatureSay(statementId, player, TALKTYPE_WHISPER, text);
				recordListener(statementId, spectatorPlayer->getGUID());
			}
		}
	}

	//event method
	for (Creature* spectator : spectators) {
		spectator->onCreatureSay(player, TALKTYPE_WHISPER, text);
	}
}

bool Game::playerYell(Player* player, const std::string& text)
{
	if (player->hasCondition(CONDITION_YELLTICKS)) {
		player->sendCancelMessage(RETURNVALUE_YOUAREEXHAUSTED);
		return false;
	}

	uint32_t minimumLevel = g_config.getNumber(ConfigManager::YELL_MINIMUM_LEVEL);
	if (player->getLevel() < minimumLevel) {
		if (g_config.getBoolean(ConfigManager::YELL_ALLOW_PREMIUM)) {
			if (player->isPremium()) {
				internalCreatureSay(player, TALKTYPE_YELL, asUpperCaseString(text), false);
				return true;
			} else {
				player->sendTextMessage(MESSAGE_STATUS_SMALL, fmt::format("You may not yell unless you have reached level {:d} or have a premium account.", minimumLevel));
			}
		} else {
			player->sendTextMessage(MESSAGE_STATUS_SMALL, fmt::format("You may not yell unless you have reached level {:d}.", minimumLevel));
		}
		return false;
	}

	if (player->getAccountType() < ACCOUNT_TYPE_GAMEMASTER) {
		Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_YELLTICKS, 30000, 0);
		player->addCondition(condition);
	}

	internalCreatureSay(player, TALKTYPE_YELL, asUpperCaseString(text), false);
	return true;
}

bool Game::playerSpeakTo(Player* player, SpeakClasses type, const std::string& receiver,
                         const std::string& text)
{
	Player* toPlayer = getPlayerByName(receiver);
	if (!toPlayer) {
		player->sendTextMessage(MESSAGE_STATUS_SMALL, "A player with this name is not online.");
		return false;
	}

	if (type != TALKTYPE_RVR_ANSWER) {
		if (type == TALKTYPE_PRIVATE_RED && !player->hasFlag(PlayerFlag_CanTalkRedPrivate)) {
			type = TALKTYPE_PRIVATE;
		}
	}

	uint32_t statementId = recordStatement(player->getGUID(), type, 0, text);
	recordListener(statementId, toPlayer->getGUID());

	toPlayer->sendPrivateMessage(player, type, text);
	toPlayer->onCreatureSay(player, type, text);

	if (toPlayer->isInGhostMode() && !player->canSeeGhostMode(toPlayer)) {
		player->sendTextMessage(MESSAGE_STATUS_SMALL, "A player with this name is not online.");
	} else {
		player->sendTextMessage(MESSAGE_STATUS_SMALL, fmt::format("Message sent to {:s}.", toPlayer->getName()));
	}
	return true;
}

void Game::processCommunication()
{
	auto timeLimit = std::time(nullptr) - 1800;
	uint32_t statementId = 0;

	while (!statements.empty())
    {
        auto it = statements.front();
        if (it.timeStamp >= timeLimit)
            break;

		statementId = it.statementID;
        statements.pop_front();
    }

	while (!listeners.empty())
    {
        auto it = listeners.front();
        if (it.StatementID >= statementId)
            break;

        listeners.pop_front();
    }

	g_scheduler.addEvent(createSchedulerTask(EVENT_COMMUNICATION_INTERVAL, std::bind(&Game::processCommunication, this)));
}

//--
bool Game::canThrowObjectTo(const Position& fromPos, const Position& toPos, bool multiFloor /*= false*/) const
{
	return map.canThrowObjectTo(fromPos, toPos, multiFloor);
}

bool Game::internalCreatureTurn(Creature* creature, Direction dir)
{
	creature->setDirection(dir);

	//send to client
	SpectatorVec spectators;
	map.getSpectators(spectators, creature->getPosition(), true, true);
	for (Creature* spectator : spectators) {
		spectator->getPlayer()->sendCreatureTurn(creature);
	}
	return true;
}

bool Game::internalCreatureSay(Creature* creature, SpeakClasses type, const std::string& text,
                               bool ghostMode, SpectatorVec* spectatorsPtr/* = nullptr*/, const Position* pos/* = nullptr*/)
{
	if (text.empty()) {
		return false;
	}

	if (!pos) {
		pos = &creature->getPosition();
	}

	uint32_t statementId = 0;

	if (Player* player = creature->getPlayer()) {
		statementId = recordStatement(player->getGUID(), type, 0, text);
	}

	SpectatorVec spectators;

	if (!spectatorsPtr || spectatorsPtr->empty()) {
		// This somewhat complex construct ensures that the cached SpectatorVec
		// is used if available and if it can be used, else a local vector is
		// used (hopefully the compiler will optimize away the construction of
		// the temporary when it's not used).
		if (type != TALKTYPE_YELL && type != TALKTYPE_MONSTER_YELL) {
			map.getSpectators(spectators, *pos, false, false,
			              Map::maxClientViewportX, Map::maxClientViewportX,
			              Map::maxClientViewportY, Map::maxClientViewportY);
		} else {
			map.getSpectators(spectators, *pos, true, false, 18, 18, 14, 14);
		}
	} else {
		spectators = (*spectatorsPtr);
	}

	//send to client
	for (Creature* spectator : spectators) {
		if (type == TALKTYPE_YELL || type == TALKTYPE_MONSTER_YELL) {
			if (pos->z >= 8 && spectator->getPosition().z != pos->z) {
				continue;
			}

			if (pos->z <= 7 && spectator->getPosition().z > 7) {
				continue;
			}
		}

		// event method
		spectator->onCreatureSay(creature, type, text);
		if (creature != spectator) {
			g_events->eventCreatureOnHear(spectator, creature, text, type);
		}

		if (Player* tmpPlayer = spectator->getPlayer()) {
			if (!ghostMode || tmpPlayer->canSeeCreature(creature)) {
				tmpPlayer->sendCreatureSay(statementId, creature, type, text, pos);
				recordListener(statementId, tmpPlayer->getGUID());
			}
		}
	}

	return true;
}

void Game::addCreatureCheck(Creature* creature)
{
	creature->creatureCheck = true;

	if (creature->inCheckCreaturesVector) {
		// already in a vector
		return;
	}

	creature->inCheckCreaturesVector = true;
	checkCreatureLists[uniform_random(0, EVENT_CREATURECOUNT - 1)].push_back(creature);
	creature->incrementReferenceCounter();
}

void Game::removeCreatureCheck(Creature* creature)
{
	if (creature->inCheckCreaturesVector) {
		creature->creatureCheck = false;
	}
}

void Game::processConditions()
{
	for (auto& i : checkCreatureLists) {
		for (auto& creature : i) {
			if (creature->creatureCheck && creature->getHealth() > 0) {
				creature->executeConditions(EVENT_CREATURE_THINK_INTERVAL);
			}
		}
	}

	g_scheduler.addEvent(createSchedulerTask(EVENT_CONDITIONS_INTERVAL, std::bind(&Game::processConditions, this)));
}

void Game::checkCreatures(size_t index)
{
	g_scheduler.addEvent(createSchedulerTask(EVENT_CHECK_CREATURE_INTERVAL, std::bind(&Game::checkCreatures, this, (index + 1) % EVENT_CREATURECOUNT)));

	auto& checkCreatureList = checkCreatureLists[index];
	auto it = checkCreatureList.begin(), end = checkCreatureList.end();
	while (it != end) {
		Creature* creature = *it;
		if (creature->creatureCheck) {
			if (creature->getHealth() > 0) {
				creature->onThink(EVENT_CREATURE_THINK_INTERVAL);
			}
			++it;
		} else {
			creature->inCheckCreaturesVector = false;
			it = checkCreatureList.erase(it);
			ReleaseCreature(creature);
		}
	}

	cleanup();
}

void Game::changeSpeed(Creature* creature, int32_t varSpeedDelta)
{
	int32_t varSpeed = creature->getVarSpeed();
	varSpeed += varSpeedDelta;
	creature->setVarSpeed(varSpeed);

	//send to clients
	SpectatorVec spectators;
	map.getSpectators(spectators, creature->getPosition(), false, true);
	for (Creature* spectator : spectators) {
		spectator->getPlayer()->sendChangeSpeed(creature, creature->getStepSpeed());
	}
}

void Game::internalCreatureChangeOutfit(Creature* creature, const Outfit_t& outfit)
{
	if (!g_events->eventCreatureOnChangeOutfit(creature, outfit)) {
		return;
	}

	creature->setCurrentOutfit(outfit);

	if (creature->isInvisible()) {
		return;
	}

	//send to clients
	SpectatorVec spectators;
	map.getSpectators(spectators, creature->getPosition(), true, true);
	for (Creature* spectator : spectators) {
		spectator->getPlayer()->sendCreatureChangeOutfit(creature, outfit);
	}
}

void Game::internalCreatureChangeVisible(Creature* creature, bool visible)
{
	//send to clients
	SpectatorVec spectators;
	map.getSpectators(spectators, creature->getPosition(), true, true);
	for (Creature* spectator : spectators) {
		spectator->getPlayer()->sendCreatureChangeVisible(creature, visible);
	}
}

void Game::changeLight(const Creature* creature)
{
	//send to clients
	SpectatorVec spectators;
	map.getSpectators(spectators, creature->getPosition(), true, true);
	for (Creature* spectator : spectators) {
		spectator->getPlayer()->sendCreatureLight(creature);
	}
}

BlockType_t Game::combatBlockHit(CombatDamage& damage, Creature* attacker, Creature* target, bool checkDefense, bool checkArmor, bool field, bool ignoreResistances /*= false */, bool meleeHit /*= false*/)
{
	if (damage.type == COMBAT_NONE) {
		return BLOCK_NONE;
	}

	if (target->getPlayer() && target->isInGhostMode() || target->getNpc()) {
		return BLOCK_NONE;
	}

	if (damage.value > 0) {
		return BLOCK_NONE;
	}

	static const auto sendBlockEffect = [this](BlockType_t blockType, CombatType_t combatType, const Position& targetPos) {
		if (blockType == BLOCK_DEFENSE) {
			addMagicEffect(targetPos, CONST_ME_POFF);
		} else if (blockType == BLOCK_ARMOR) {
			addMagicEffect(targetPos, CONST_ME_BLOCKHIT);
		} else if (blockType == BLOCK_IMMUNITY) {
			uint8_t hitEffect = 0;
			switch (combatType) {
				case COMBAT_UNDEFINEDDAMAGE: {
					return;
				}
				case COMBAT_EARTHDAMAGE:
				case COMBAT_ENERGYDAMAGE:
				case COMBAT_FIREDAMAGE:
				case COMBAT_PHYSICALDAMAGE: {
					hitEffect = CONST_ME_BLOCKHIT;
					break;
				}
				default: {
					hitEffect = CONST_ME_POFF;
					break;
				}
			}
			addMagicEffect(targetPos, hitEffect);
		}
	};

	BlockType_t blockType;
	if (damage.type != COMBAT_NONE) {
		damage.value = -damage.value;
		blockType = target->blockHit(attacker, damage.type, damage.value, checkDefense, checkArmor, field, ignoreResistances, meleeHit);

		damage.value = -damage.value;
		
		// we're already taking care of the effects inside the onStepInField call
		if (!field) {
			sendBlockEffect(blockType, damage.type, target->getPosition());
		}
	} else {
		blockType = BLOCK_NONE;
	}

	damage.blockType = blockType;
	return damage.blockType;
}

void Game::combatGetTypeInfo(CombatType_t combatType, Creature* target, TextColor_t& color, uint8_t& effect)
{
	switch (combatType) {
		case COMBAT_PHYSICALDAMAGE: {
			Item* splash = nullptr;
			switch (target->getRace()) {
				case RACE_VENOM:
					color = TEXTCOLOR_LIGHTGREEN;
					effect = CONST_ME_HITBYPOISON;
					splash = Item::CreateItem(ITEM_SMALLSPLASH, FLUID_SLIME);
					break;
				case RACE_BLOOD:
					color = TEXTCOLOR_RED;
					effect = CONST_ME_DRAWBLOOD;
					if (const Tile* tile = target->getTile()) {
						if (!tile->hasFlag(TILESTATE_PROTECTIONZONE)) {
							splash = Item::CreateItem(ITEM_SMALLSPLASH, FLUID_BLOOD);
						}
					}
					break;
				case RACE_UNDEAD:
					color = TEXTCOLOR_LIGHTGREY;
					effect = CONST_ME_HITAREA;
					break;
				case RACE_FIRE:
					color = TEXTCOLOR_ORANGE;
					effect = CONST_ME_DRAWBLOOD;
					break;
				case RACE_ENERGY:
					color = TEXTCOLOR_LIGHTBLUE;
					effect = CONST_ME_ENERGYHIT;
					break;
				default:
					color = TEXTCOLOR_NONE;
					effect = CONST_ME_NONE;
					break;
			}

			if (splash) {
				// Does not allow to create pools on tiles with a "Bottom" item
				// And remove previous splash if there is any
				Item* previousSplash = target->getTile()->getSplashItem();
				if (previousSplash) {
					g_game.internalRemoveItem(previousSplash);
				}

				if (!target->getTile()->getItemByTopOrder(2)) {
					internalAddItem(target->getTile(), splash, INDEX_WHEREEVER, FLAG_NOLIMIT);
					startDecay(splash);
				} else {
					delete splash;
				}
			}

			break;
		}

		case COMBAT_ENERGYDAMAGE: {
			color = TEXTCOLOR_LIGHTBLUE;
			effect = CONST_ME_ENERGYHIT;
			break;
		}

		case COMBAT_EARTHDAMAGE: {
			color = TEXTCOLOR_LIGHTGREEN;
			effect = CONST_ME_GREEN_RINGS;
			break;
		}

		case COMBAT_FIREDAMAGE: {
			color = TEXTCOLOR_ORANGE;
			effect = CONST_ME_HITBYFIRE;
			break;
		}
		case COMBAT_LIFEDRAIN: {
			color = TEXTCOLOR_RED;
			effect = CONST_ME_MAGIC_RED;
			break;
		}
		default: {
			color = TEXTCOLOR_NONE;
			effect = CONST_ME_NONE;
			break;
		}
	}
}

bool Game::combatChangeHealth(Creature* attacker, Creature* target, CombatDamage& damage)
{
	if (g_config.getBoolean(ConfigManager::UNLIMITED_PLAYER_HP) && target->getPlayer()) {
		return true;
	}

	const Position& targetPos = target->getPosition();
	if (damage.value > 0) {
		if (target->getHealth() <= 0) {
			return false;
		}

		if (damage.origin != ORIGIN_NONE) {
			const auto& events = target->getCreatureEvents(CREATURE_EVENT_HEALTHCHANGE);
			if (!events.empty()) {
				for (CreatureEvent* creatureEvent : events) {
					creatureEvent->executeHealthChange(target, attacker, damage);
				}
				damage.origin = ORIGIN_NONE;
				return combatChangeHealth(attacker, target, damage);
			}
		}

		target->gainHealth(attacker, damage.value);
	} else {
		if (!target->isAttackable()) {
			if (!target->isInGhostMode() && !target->getNpc()) {
				addMagicEffect(targetPos, CONST_ME_POFF);
			}
			return true;
		}

		Player* attackerPlayer;
		if (attacker) {
			attackerPlayer = attacker->getPlayer();
		} else {
			attackerPlayer = nullptr;
		}

		Player* targetPlayer = target->getPlayer();

		damage.value = std::abs(damage.value);

		int32_t healthChange = damage.value;
		if (healthChange == 0) {
			return true;
		}

		if (targetPlayer && target->hasCondition(CONDITION_MANASHIELD) && damage.type != COMBAT_UNDEFINEDDAMAGE) {
			if (targetPlayer->getMana() >= healthChange) {
				if (damage.origin != ORIGIN_NONE) {
					const auto& events = target->getCreatureEvents(CREATURE_EVENT_MANACHANGE);
					if (!events.empty()) {
						for (CreatureEvent* creatureEvent : events) {
							creatureEvent->executeManaChange(target, attacker, damage);
						}
						healthChange = damage.value;
						if (healthChange == 0) {
							return true;
						}
					}
				}

				targetPlayer->drainMana(attacker, healthChange);

				addMagicEffect(targetPos, CONST_ME_LOSEENERGY);
				addAnimatedText(targetPos, TEXTCOLOR_BLUE, std::to_string(damage.value));

				TextMessage message;
				message.type = MESSAGE_EVENT_DEFAULT;
				if (!attacker) {
					message.text = fmt::format("You lose {:d} mana.", healthChange);
				}
				else {
					message.text = fmt::format("You lose {:d} mana due to an attack by {:s}.", healthChange, attacker->getNameDescription());
				}
				targetPlayer->sendTextMessage(message);
				return true;
			}

			healthChange -= targetPlayer->getMana();
			targetPlayer->drainMana(attacker, targetPlayer->getMana());

			damage.value = healthChange;
		}

		int32_t realDamage = damage.value;
		if (realDamage == 0) {
			return true;
		}

		if (damage.origin != ORIGIN_NONE) {
			const auto& events = target->getCreatureEvents(CREATURE_EVENT_HEALTHCHANGE);
			if (!events.empty()) {
				for (CreatureEvent* creatureEvent : events) {
					creatureEvent->executeHealthChange(target, attacker, damage);
				}
				damage.origin = ORIGIN_NONE;
				return combatChangeHealth(attacker, target, damage);
			}
		}

		int32_t targetHealth = target->getHealth();
		if (damage.value >= targetHealth) {
			damage.value = targetHealth;
		}

		realDamage = damage.value;
		if (realDamage == 0) {
			return true;
		}

		TextColor_t textColor = TEXTCOLOR_NONE;

		uint8_t hitEffect;
		if (damage.value) {
			combatGetTypeInfo(damage.type, target, textColor, hitEffect);
			if (hitEffect != CONST_ME_NONE) {
				addMagicEffect(targetPos, hitEffect);
				addAnimatedText(targetPos, textColor, std::to_string(damage.value));
			}
		}

		if (targetPlayer && textColor != TEXTCOLOR_NONE) {
			auto damageString = fmt::format("{:d} hitpoint{:s}", realDamage, realDamage != 1 ? "s" : "");

			std::string spectatorMessage;

			TextMessage message;
			message.type = MESSAGE_EVENT_DEFAULT;
			if (!attacker) {
				message.text = fmt::format("You lose {:s}.", damageString);
			} else {
				message.text = fmt::format("You lose {:s} due to an attack by {:s}.", damageString, attacker->getNameDescription());
			}
			targetPlayer->sendTextMessage(message);
		}

		if (realDamage >= targetHealth) {
			for (CreatureEvent* creatureEvent : target->getCreatureEvents(CREATURE_EVENT_PREPAREDEATH)) {
				if (!creatureEvent->executeOnPrepareDeath(target, attacker)) {
					return false;
				}
			}
		}

		target->drainHealth(attacker, realDamage);
		addCreatureHealth(target);
	}

	return true;
}

bool Game::combatChangeMana(Creature* attacker, Creature* target, CombatDamage& damage)
{
	Player* targetPlayer = target->getPlayer();
	if (!targetPlayer) {
		return true;
	}

	int32_t manaChange = damage.value;
	if (manaChange > 0) {
		if (damage.origin != ORIGIN_NONE) {
			const auto& events = target->getCreatureEvents(CREATURE_EVENT_MANACHANGE);
			if (!events.empty()) {
				for (CreatureEvent* creatureEvent : events) {
					creatureEvent->executeManaChange(target, attacker, damage);
				}
				damage.origin = ORIGIN_NONE;
				return combatChangeMana(attacker, target, damage);
			}
		}

		targetPlayer->changeMana(manaChange);
	} else {
		const Position& targetPos = target->getPosition();
		if (!target->isAttackable()) {
			if (!target->isInGhostMode()) {
				addMagicEffect(targetPos, CONST_ME_POFF);
			}
			return false;
		}

		Player* attackerPlayer;
		if (attacker) {
			attackerPlayer = attacker->getPlayer();
		} else {
			attackerPlayer = nullptr;
		}

		int32_t manaLoss = std::min<int32_t>(targetPlayer->getMana(), -manaChange);
		BlockType_t blockType = target->blockHit(attacker, COMBAT_MANADRAIN, manaLoss);
		if (blockType != BLOCK_NONE) {
			addMagicEffect(targetPos, CONST_ME_POFF);
			return false;
		}

		if (manaLoss <= 0) {
			return true;
		}

		if (damage.origin != ORIGIN_NONE) {
			const auto& events = target->getCreatureEvents(CREATURE_EVENT_MANACHANGE);
			if (!events.empty()) {
				for (CreatureEvent* creatureEvent : events) {
					creatureEvent->executeManaChange(target, attacker, damage);
				}
				damage.origin = ORIGIN_NONE;
				return combatChangeMana(attacker, target, damage);
			}
		}

		targetPlayer->drainMana(attacker, manaLoss);
		addAnimatedText(targetPos, TEXTCOLOR_BLUE, std::to_string(manaLoss));

		TextMessage message;
		message.type = MESSAGE_EVENT_DEFAULT;
		if (!attacker) {
			message.text = fmt::format("You lose {:d} mana.", manaLoss);
		} else {
			message.text = fmt::format("You lose {:d} mana due to an attack by {:s}.", manaLoss, attacker->getNameDescription());
		}

		targetPlayer->sendTextMessage(message);
	}

	return true;
}

void Game::addCreatureHealth(const Creature* target)
{
	SpectatorVec spectators;
	map.getSpectators(spectators, target->getPosition(), true, true);
	addCreatureHealth(spectators, target);
}

void Game::addCreatureHealth(const SpectatorVec& spectators, const Creature* target)
{
	for (Creature* spectator : spectators) {
		if (Player* tmpPlayer = spectator->getPlayer()) {
			tmpPlayer->sendCreatureHealth(target);
		}
	}
}

void Game::addMagicEffect(const Position& pos, uint8_t effect)
{
	SpectatorVec spectators;
	map.getSpectators(spectators, pos, true, true, 16, 16, 14, 14);
	addMagicEffect(spectators, pos, effect);
}

void Game::addMagicEffect(const SpectatorVec& spectators, const Position& pos, uint8_t effect)
{
	for (Creature* spectator : spectators) {
		if (Player* tmpPlayer = spectator->getPlayer()) {
			tmpPlayer->sendMagicEffect(pos, effect);
		}
	}
}

void Game::addDistanceEffect(const Position& fromPos, const Position& toPos, uint8_t effect)
{
	SpectatorVec spectators, toPosSpectators;
	map.getSpectators(spectators, fromPos, false, true);
	map.getSpectators(toPosSpectators, toPos, false, true);
	spectators.addSpectators(toPosSpectators);

	addDistanceEffect(spectators, fromPos, toPos, effect);
}

void Game::addDistanceEffect(const SpectatorVec& spectators, const Position& fromPos, const Position& toPos, uint8_t effect)
{
	for (Creature* spectator : spectators) {
		if (Player* tmpPlayer = spectator->getPlayer()) {
			tmpPlayer->sendDistanceShoot(fromPos, toPos, effect);
		}
	}
}

void Game::addAnimatedText(const Position& pos, TextColor_t textColor, const std::string& text)
{
	SpectatorVec spectators;
	map.getSpectators(spectators, pos, false, true);

	addAnimatedText(spectators, pos, textColor, text);
}

void Game::addAnimatedText(const SpectatorVec& spectators, const Position& pos, TextColor_t textColor, const std::string& text)
{
	for (Creature* spectator : spectators) {
		if (Player* tmpPlayer = spectator->getPlayer()) {
			tmpPlayer->sendAnimatedText(pos, textColor, text);
		}
	}
}

void Game::setAccountStorageValue(const uint32_t accountId, const uint32_t key, const int32_t value)
{
	if (value == -1) {
		accountStorageMap[accountId].erase(key);
		return;
	}

	accountStorageMap[accountId][key] = value;
}

int32_t Game::getAccountStorageValue(const uint32_t accountId, const uint32_t key) const
{
	const auto& accountMapIt = accountStorageMap.find(accountId);
	if (accountMapIt != accountStorageMap.end()) {
		const auto& storageMapIt = accountMapIt->second.find(key);
		if (storageMapIt != accountMapIt->second.end()) {
			return storageMapIt->second;
		}
	}
	return -1;
}

void Game::loadAccountStorageValues()
{
	Database& db = Database::getInstance();

	DBResult_ptr result;
	if ((result = db.storeQuery("SELECT `account_id`, `key`, `value` FROM `account_storage`"))) {
		do {
			g_game.setAccountStorageValue(result->getNumber<uint32_t>("account_id"), result->getNumber<uint32_t>("key"), result->getNumber<int32_t>("value"));
		} while (result->next());
	}
}

bool Game::saveAccountStorageValues() const
{
	DBTransaction transaction;
	Database& db = Database::getInstance();

	if (!transaction.begin()) {
		return false;
	}

	if (!db.executeQuery("DELETE FROM `account_storage`")) {
		return false;
	}

	for (const auto& accountIt : g_game.accountStorageMap) {
		if (accountIt.second.empty()) {
			continue;
		}

		DBInsert accountStorageQuery("INSERT INTO `account_storage` (`account_id`, `key`, `value`) VALUES");
		for (const auto& storageIt : accountIt.second) {
			if (!accountStorageQuery.addRow(fmt::format("{:d}, {:d}, {:d}", accountIt.first, storageIt.first, storageIt.second))) {
				return false;
			}
		}

		if (!accountStorageQuery.execute()) {
			return false;
		}
	}

	return transaction.commit();
}

void Game::startDecay(Item* item)
{
	if (!item || !item->canDecay()) {
		return;
	}

	ItemDecayState_t decayState = item->getDecaying();
	if (decayState == DECAYING_TRUE) {
		return;
	}

	if (item->getDuration() > 0) {
		item->incrementReferenceCounter();
		item->setDecaying(DECAYING_TRUE);
		toDecayItems.push_front(item);
	} else {
		internalDecayItem(item);
	}
}

void Game::internalDecayItem(Item* item)
{
	const ItemType& it = Item::items[item->getID()];
	if (it.decayTo != 0) {
		Item* newItem = transformItem(item, item->getDecayTo());
		startDecay(newItem);
	} else {
		ReturnValue ret = internalRemoveItem(item);
		if (ret != RETURNVALUE_NOERROR) {
			std::cout << "[Debug - Game::internalDecayItem] internalDecayItem failed, error code: " << static_cast<uint32_t>(ret) << ", item id: " << item->getID() << std::endl;
		}
	}
}

void Game::processRemovedCreatures()
{
	// process killed creatures
	for (Creature* creature : killedCreatures) {
		if (creature->isRemoved()) { // removed creatures cannot execute death scripts
			continue;
		}

		creature->onDeath();
		creature->decrementReferenceCounter();
	}
	killedCreatures.clear();

	// process removed creatures
	for (Creature* creature : removedCreatures) {
		if (creature->isRemoved()) { // removed creatures cannot disconnect from the game
			continue;
		}

		addMagicEffect(creature->getPosition(), CONST_ME_POFF);
		if (Player* player = creature->getPlayer()) {
			player->disconnect();
		}
		removeCreature(creature);
		creature->decrementReferenceCounter();
	}
	removedCreatures.clear();

	g_scheduler.addEvent(createSchedulerTask(1000, std::bind(&Game::processRemovedCreatures, this)));
}

void Game::proceduralRefreshMap()
{
	if (!g_config.getBoolean(ConfigManager::ENABLE_MAP_REFRESH) || getGameState() >= GAME_STATE_SHUTDOWN) {
		return;
	}

	if (tilesToRefresh.size() > 0) {
		bool increaseRefreshSet = true;
		int32_t tilesPerCycle = g_config.getNumber(ConfigManager::MAP_REFRESH_TILES_PER_CYCLE);
		for (int32_t i = nextMapRefreshSet; i <= nextMapRefreshSet + tilesPerCycle; i++) {
			if (i == tilesToRefresh.size() - 1) {
				nextMapRefreshSet = 0;
				increaseRefreshSet = false;
				break;
			}

			// skip procedural map refresh upon ending game
			if (getGameState() >= GAME_STATE_SHUTDOWN) {
				return;
			}

			Tile* tile = tilesToRefresh[i];
			if (!tile || tile->getCreatureCount() > 0) {
				continue;
			}

			if (OTSYS_TIME() < tile->getNextRefreshTime()) {
				// this tile cannot refresh at this moment
				continue;
			}

			SpectatorVec spectators;
			map.getSpectators(spectators, tile->getPosition(), true, true, 16, 16, 16, 16);

			if (!spectators.empty()) {
				// cannot refresh this area, there are players/creatures present
				continue;
			}

			tile->refresh();
			tile->updateRefreshTime();
		}

		if (increaseRefreshSet) {
			nextMapRefreshSet += g_config.getNumber(ConfigManager::MAP_REFRESH_TILES_PER_CYCLE);
		}
	}

	eventRefreshId = g_scheduler.addEvent(createSchedulerTask(g_config.getNumber(ConfigManager::MAP_REFRESH_INTERVAL), std::bind(&Game::proceduralRefreshMap, this)));
}

void Game::checkDecay()
{
	g_scheduler.addEvent(createSchedulerTask(EVENT_DECAYINTERVAL, std::bind(&Game::checkDecay, this)));

	size_t bucket = (lastBucket + 1) % EVENT_DECAY_BUCKETS;

	auto it = decayItems[bucket].begin(), end = decayItems[bucket].end();
	while (it != end) {
		Item* item = *it;
		if (!item->canDecay()) {
			item->setDecaying(DECAYING_FALSE);
			ReleaseItem(item);
			it = decayItems[bucket].erase(it);
			continue;
		}

		int32_t duration = item->getDuration();
		int32_t decreaseTime = std::min<int32_t>(EVENT_DECAYINTERVAL * EVENT_DECAY_BUCKETS, duration);

		duration -= decreaseTime;
		item->decreaseDuration(decreaseTime);

		if (duration <= 0) {
			it = decayItems[bucket].erase(it);
			internalDecayItem(item);
			ReleaseItem(item);
		} else if (duration < EVENT_DECAYINTERVAL * EVENT_DECAY_BUCKETS) {
			it = decayItems[bucket].erase(it);
			size_t newBucket = (bucket + ((duration + EVENT_DECAYINTERVAL / 2) / 1000)) % EVENT_DECAY_BUCKETS;
			if (newBucket == bucket) {
				internalDecayItem(item);
				ReleaseItem(item);
			} else {
				decayItems[newBucket].push_back(item);
			}
		} else {
			++it;
		}
	}

	lastBucket = bucket;
	cleanup();
}

void Game::checkLight()
{
	g_scheduler.addEvent(createSchedulerTask(EVENT_LIGHTINTERVAL, std::bind(&Game::checkLight, this)));
	oldLightLevel = lightLevel;
	oldLightColor = lightColor;
	updateWorldLightLevel();

	if (oldLightLevel != lightLevel || oldLightColor != lightColor) {
		LightInfo lightInfo = getWorldLightInfo();

		for (const auto& it : players) {
			it.second->sendWorldLight(lightInfo);
		}
	}
}

void Game::updateWorldLightLevel()
{
	const time_t timer = time(0);
	const tm* localTime = localtime(&timer);
	const int32_t timeInMinutes = localTime->tm_sec + 60 * localTime->tm_min;
	const int32_t timeSegment = 2 * (timeInMinutes % 150) / 5 + 60 * (timeInMinutes / 150);

	if (timeSegment <= 59 || timeSegment > 1380)
	{
		lightLevel = 51;
		lightColor = 215;
	}
	else if (timeSegment <= 119 || (timeSegment > 1320 && timeSegment <= 1380))
	{
		lightLevel = 102;
		lightColor = 215;
	}
	else if (timeSegment <= 179)
	{
		lightLevel = 153;
		lightColor = 173;
	}
	else if (timeSegment <= 239)
	{
		lightLevel = 204;
		lightColor = 173;
	}
	else if (timeSegment <= 1200)
	{
		lightLevel = 255;
		lightColor = 215;
	}
	else if (timeSegment <= 1260)
	{
		lightLevel = 204;
		lightColor = 208;
	}
	else
	{
		lightLevel = 153;
		lightColor = 208;
	}
}

void Game::updateWorldTime()
{
	g_scheduler.addEvent(createSchedulerTask(EVENT_WORLDTIMEINTERVAL, std::bind(&Game::updateWorldTime, this)));
	time_t osTime = time(nullptr);
	tm* timeInfo = localtime(&osTime);
	worldTime = (timeInfo->tm_sec + (timeInfo->tm_min * 60)) / 2.5f;
}

void Game::shutdown()
{
	std::cout << "Shutting down..." << std::flush;

	g_scheduler.shutdown();
	g_databaseTasks.shutdown();
	g_dispatcher.shutdown();
	map.spawns.clear();
	raids.clear();

	cleanup();

	if (serviceManager) {
		serviceManager->stop();
	}

	ConnectionManager::getInstance().closeAll();

	std::cout << " done!" << std::endl;
}

void Game::cleanup()
{
	//free memory
	for (auto creature : ToReleaseCreatures) {
		if (creature == nullptr) { // TFS is buggy
			std::cout << "[Game::cleanup] -> Removed Creature is NULL! Skipping..." << std::endl;
			continue;
		}

		creature->decrementReferenceCounter();
	}
	ToReleaseCreatures.clear();

	for (auto item : ToReleaseItems) {
		item->decrementReferenceCounter();
	}
	ToReleaseItems.clear();

	for (Item* item : toDecayItems) {
		const uint32_t dur = item->getDuration();
		if (dur >= EVENT_DECAYINTERVAL * EVENT_DECAY_BUCKETS) {
			decayItems[lastBucket].push_back(item);
		} else {
			decayItems[(lastBucket + 1 + dur / 1000) % EVENT_DECAY_BUCKETS].push_back(item);
		}
	}
	toDecayItems.clear();
}

void Game::ReleaseCreature(Creature* creature)
{
	ToReleaseCreatures.push_back(creature);
}

void Game::ReleaseItem(Item* item)
{
	ToReleaseItems.push_back(item);
}

void Game::broadcastMessage(const std::string& text, MessageClasses type) const
{
	std::cout << "> Broadcasted message: \"" << text << "\"." << std::endl;
	for (const auto& it : players) {
		it.second->sendTextMessage(type, text);
	}
}

void Game::executeCreature(uint32_t creatureId)
{
	Creature* creature = getCreatureByID(creatureId);
	if (!creature || creature->isRemoved() || creature->toDoEntries.empty()) {
		return;
	}

	creature->executeToDoEntries();
}

void Game::updateCreatureSkull(const Creature* creature)
{
	if (getWorldType() != WORLD_TYPE_PVP) {
		return;
	}

	SpectatorVec spectators;
	map.getSpectators(spectators, creature->getPosition(), true, true);
	for (Creature* spectator : spectators) {
		spectator->getPlayer()->sendCreatureSkull(creature);
	}
}

void Game::updatePlayerShield(Player* player)
{
	SpectatorVec spectators;
	map.getSpectators(spectators, player->getPosition(), true, true);
	for (Creature* spectator : spectators) {
		spectator->getPlayer()->sendCreatureShield(player);
	}
}

void Game::loadMotdNum()
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery("SELECT `value` FROM `server_config` WHERE `config` = 'motd_num'");
	if (result) {
		motdNum = result->getNumber<uint32_t>("value");
	} else {
		db.executeQuery("INSERT INTO `server_config` (`config`, `value`) VALUES ('motd_num', '0')");
	}

	result = db.storeQuery("SELECT `value` FROM `server_config` WHERE `config` = 'motd_hash'");
	if (result) {
		motdHash = result->getString("value");
		if (motdHash != transformToSHA1(g_config.getString(ConfigManager::MOTD))) {
			++motdNum;
		}
	} else {
		db.executeQuery("INSERT INTO `server_config` (`config`, `value`) VALUES ('motd_hash', '')");
	}
}

void Game::saveMotdNum() const
{
	Database& db = Database::getInstance();
	db.executeQuery(fmt::format("UPDATE `server_config` SET `value` = '{:d}' WHERE `config` = 'motd_num'", motdNum));
	db.executeQuery(fmt::format("UPDATE `server_config` SET `value` = '{:s}' WHERE `config` = 'motd_hash'", transformToSHA1(g_config.getString(ConfigManager::MOTD))));
}

void Game::checkPlayersRecord()
{
	const size_t playersOnline = getPlayersOnline();
	if (playersOnline > playersRecord) {
		uint32_t previousRecord = playersRecord;
		playersRecord = playersOnline;

		for (auto& it : g_globalEvents->getEventMap(GLOBALEVENT_RECORD)) {
			it.second.executeRecord(playersRecord, previousRecord);
		}
		updatePlayersRecord();
	}
}

void Game::updatePlayersRecord() const
{
	Database& db = Database::getInstance();
	db.executeQuery(fmt::format("UPDATE `server_config` SET `value` = '{:d}' WHERE `config` = 'players_record'", playersRecord));
}

void Game::loadPlayersRecord()
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery("SELECT `value` FROM `server_config` WHERE `config` = 'players_record'");
	if (result) {
		playersRecord = result->getNumber<uint32_t>("value");
	} else {
		db.executeQuery("INSERT INTO `server_config` (`config`, `value`) VALUES ('players_record', '0')");
	}
}

void Game::playerInviteToParty(uint32_t playerId, uint32_t invitedId)
{
	if (playerId == invitedId) {
		return;
	}

	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Player* invitedPlayer = getPlayerByID(invitedId);
	if (!invitedPlayer || invitedPlayer->isInviting(player)) {
		return;
	}

	if (!Position::areInRange<8, 8>(invitedPlayer->getPosition(), player->getPosition())) {
		return;
	}

	if (invitedPlayer->getParty()) {
		player->sendTextMessage(MESSAGE_INFO_DESCR, fmt::format("{:s} is already in a party.", invitedPlayer->getName()));
		return;
	}

	Party* party = player->getParty();
	if (!party) {
		party = new Party(player);
	} else if (party->getLeader() != player) {
		return;
	}

	party->invitePlayer(*invitedPlayer);
}

void Game::playerJoinParty(uint32_t playerId, uint32_t leaderId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Player* leader = getPlayerByID(leaderId);
	if (!leader || !leader->isInviting(player)) {
		return;
	}

	Party* party = leader->getParty();
	if (!party || party->getLeader() != leader) {
		return;
	}

	if (player->getParty()) {
		player->sendTextMessage(MESSAGE_INFO_DESCR, "You are already in a party.");
		return;
	}

	party->joinParty(*player);
}

void Game::playerRevokePartyInvitation(uint32_t playerId, uint32_t invitedId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Party* party = player->getParty();
	if (!party || party->getLeader() != player) {
		return;
	}

	Player* invitedPlayer = getPlayerByID(invitedId);
	if (!invitedPlayer || !player->isInviting(invitedPlayer)) {
		return;
	}

	party->revokeInvitation(*invitedPlayer);
}

void Game::playerPassPartyLeadership(uint32_t playerId, uint32_t newLeaderId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Party* party = player->getParty();
	if (!party || party->getLeader() != player) {
		return;
	}

	Player* newLeader = getPlayerByID(newLeaderId);
	if (!newLeader || !player->isPartner(newLeader)) {
		return;
	}

	party->passPartyLeadership(newLeader);
}

void Game::playerLeaveParty(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Party* party = player->getParty();
	if (!party || player->hasCondition(CONDITION_INFIGHT)) {
		return;
	}

	party->leaveParty(player);
}

void Game::playerEnableSharedPartyExperience(uint32_t playerId, bool sharedExpActive)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Party* party = player->getParty();
	if (!party || (player->hasCondition(CONDITION_INFIGHT) && player->getZone() != ZONE_PROTECTION)) {
		return;
	}

	party->setSharedExperience(player, sharedExpActive);
}

void Game::playerProcessRuleViolationReport(uint32_t playerId, const std::string& name)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	if (player->getAccountType() < ACCOUNT_TYPE_GAMEMASTER) {
		return;
	}

	Player* reporter = getPlayerByName(name);
	if (!reporter) {
		return;
	}

	auto it = ruleViolations.find(reporter->getID());
	if (it == ruleViolations.end()) {
		return;
	}

	RuleViolation& ruleViolation = it->second;
	if (!ruleViolation.pending) {
		return;
	}

	ruleViolation.gamemasterId = player->getID();
	ruleViolation.pending = false;

	ChatChannel* channel = g_chat->getChannelById(CHANNEL_RULE_REP);
	if (channel) {
		for (auto userPtr : channel->getUsers()) {
			if (userPtr.second) {
				userPtr.second->sendRemoveRuleViolationReport(reporter->getName());
			}
		}
	}
}

void Game::playerCloseRuleViolationReport(uint32_t playerId, const std::string& name)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Player* reporter = getPlayerByName(name);
	if (!reporter) {
		return;
	}

	closeRuleViolationReport(reporter);
}

void Game::playerCancelRuleViolationReport(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	cancelRuleViolationReport(player);
}

void Game::playerReportRuleViolationReport(Player* player, const std::string& text)
{
	auto it = ruleViolations.find(player->getID());
	if (it != ruleViolations.end()) {
		player->sendCancelMessage("You already have a pending rule violation report. Close it before starting a new one.");
		return;
	}

	RuleViolation ruleViolation = RuleViolation(player->getID(), text);
	ruleViolation.timestamp = std::time(nullptr);
	ruleViolations[player->getID()] = ruleViolation;

	ChatChannel* channel = g_chat->getChannelById(CHANNEL_RULE_REP);
	if (channel) {
		for (auto userPtr : channel->getUsers()) {
			if (userPtr.second) {
				userPtr.second->sendToChannel(0, player, TALKTYPE_RVR_CHANNEL, text, CHANNEL_RULE_REP);
			}
		}
	}
}

void Game::playerContinueRuleViolationReport(Player* player, const std::string& text)
{
	auto it = ruleViolations.find(player->getID());
	if (it == ruleViolations.end()) {
		return;
	}

	RuleViolation& rvr = it->second;
	Player* toPlayer = getPlayerByID(rvr.gamemasterId);
	if (!toPlayer) {
		return;
	}

	toPlayer->sendPrivateMessage(player, TALKTYPE_RVR_CONTINUE, text);
	player->sendTextMessage(MESSAGE_STATUS_SMALL, "Message sent to Counsellor.");
}

void Game::closeRuleViolationReport(Player* player)
{
	const auto it = ruleViolations.find(player->getID());
	if (it == ruleViolations.end()) {
		return;
	}

	ruleViolations.erase(it);
	player->sendLockRuleViolationReport();

	ChatChannel* channel = g_chat->getChannelById(CHANNEL_RULE_REP);
	if (channel) {
		for (UsersMap::const_iterator ut = channel->getUsers().begin(); ut != channel->getUsers().end(); ++ut) {
			if (ut->second) {
				ut->second->sendRemoveRuleViolationReport(player->getName());
			}
		}
	}
}

void Game::cancelRuleViolationReport(Player* player)
{
	const auto it = ruleViolations.find(player->getID());
	if (it == ruleViolations.end()) {
		return;
	}

	RuleViolation& ruleViolation = it->second;
	Player* gamemaster = getPlayerByID(ruleViolation.gamemasterId);
	if (!ruleViolation.pending && gamemaster) {
		// Send to the responder
		gamemaster->sendRuleViolationCancel(player->getName());
	}

	// Send to channel
	ChatChannel* channel = g_chat->getChannelById(CHANNEL_RULE_REP);
	if (channel) {
		for (UsersMap::const_iterator ut = channel->getUsers().begin(); ut != channel->getUsers().end(); ++ut) {
			if (ut->second) {
				ut->second->sendRemoveRuleViolationReport(player->getName());
			}
		}
	}

	// Erase it
	ruleViolations.erase(it);
}

void Game::sendGuildMotd(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	Guild* guild = player->getGuild();
	if (guild) {
		player->sendChannelMessage("Message of the Day", guild->getMotd(), TALKTYPE_CHANNEL_R1, CHANNEL_GUILD);
	}
}

void Game::kickPlayer(uint32_t playerId, bool displayEffect)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	player->kickPlayer(displayEffect, true);
}

void Game::playerReportBug(uint32_t playerId, const std::string& message)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	g_events->eventPlayerOnReportBug(player, message);
}

void Game::playerDebugAssert(uint32_t playerId, const std::string& assertLine, const std::string& date, const std::string& description, const std::string& comment)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	// TODO: move debug assertions to database
	FILE* file = fopen("gamedata/client_assertions.txt", "a");
	if (file) {
		fprintf(file, "----- %s - %s (%s) -----\n", formatDate(time(nullptr)).c_str(), player->getName().c_str(), convertIPToString(player->getIP()).c_str());
		fprintf(file, "%s\n%s\n%s\n%s\n", assertLine.c_str(), date.c_str(), description.c_str(), comment.c_str());
		fclose(file);
	}
}

void Game::parsePlayerExtendedOpcode(uint32_t playerId, uint8_t opcode, const std::string& buffer)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	for (CreatureEvent* creatureEvent : player->getCreatureEvents(CREATURE_EVENT_EXTENDED_OPCODE)) {
		creatureEvent->executeExtendedOpcode(player, opcode, buffer);
	}
}

bool Game::jumpPossible(int32_t x, int32_t y, int32_t z, bool avoidPlayers)
{
	Tile* tile = map.getTile(x, y, z);

	if (tile) {
		if (!tile->getGround()) {
			return false;
		}

		if (tile->hasFlag(TILESTATE_IMMOVABLEBLOCKSOLID)) {
			return false;
		}

		if (avoidPlayers) {
			if (tile->getTopCreature() && tile->getTopCreature()->getPlayer()) {
				return false;
			}
		}
	}

	return true;
}

bool Game::searchFreeField(Creature* creature, uint16_t& x, uint16_t& y, uint8_t& z, int32_t distance, bool jump, bool allowHouses)
{
	int32_t Direction = 4;
	int32_t Dist = 0;
	int32_t dx = 0;
	int32_t dy = 0;

	while (true) {
		Tile* tile = map.getTile(x + dx, y + dy, z);
		if (!tile || !tile->getGround() || tile->getCreatureCount() > 0 || tile->hasFlag(TILESTATE_BLOCKSOLID) || tile->hasFlag(TILESTATE_BLOCKPATH) ||
			creature && tile->queryAdd(INDEX_WHEREEVER, *creature, 1, FLAG_PATHFINDING) != RETURNVALUE_NOERROR || tile->getHouse() != nullptr && !allowHouses) {
			if (Direction == 2) {
				--dx;
				if (!(Dist + dx)) {
					Direction = 3;
				}
			}
			else if (Direction > 2) {
				if (Direction == 3) {
					if (++dy == Dist) {
						Direction = 4;
					}
				}
				else if (++dx == Dist + 1) {
					Direction = 1;
					Dist = dx;
				}
			}
			else {
				--dy;
				if (!(Dist + dy)) {
					Direction = 2;
				}
			}

			if (Dist > distance) {
				return false;
			}
		}
		else {
			break;
		}
	}

	while (jump) {
		bool canJump = jumpPossible(x + dx, y + dy, z, true);
		if (canJump) {
			break;
		}

		if (Direction == 2) {
			--dx;
			if (!(Dist + dx)) {
				Direction = 3;
			}
		}
		else if (Direction > 2) {
			if (Direction == 3) {
				if (++dy == Dist) {
					Direction = 4;
				}
			}
			else if (++dx == Dist + 1) {
				Direction = 1;
				Dist = dx;
			}
		}
		else {
			--dy;
			if (!(Dist + dy)) {
				Direction = 2;
			}
		}

		if (Dist > distance) {
			return false;
		}

		Tile* tile = map.getTile(x + dx, y + dy, z);
		if (!tile) {
			continue;
		}

		if (tile->hasFlag(TILESTATE_BLOCKSOLID)) {
			continue;
		}

		if (tile->hasFlag(TILESTATE_BLOCKPATH)) {
			continue;
		}

		if (!tile->getGround()) {
			continue;
		}

		break;
	}

	x += dx;
	y += dy;
	return true;
}

bool Game::searchLoginField(Creature* creature, uint16_t& x, uint16_t& y, uint8_t& z, int32_t distance, bool player, bool allowHouses)
{
	Tile* targetTile = map.getTile(x, y, z);
	bool noLogoutField = targetTile && targetTile->hasFlag(TILESTATE_NOLOGOUT);

	if (searchFreeField(creature, x, y, z, distance, false, allowHouses) && (!player || !noLogoutField)) {
		return true;
	}

	int32_t Direction = 4;
	int32_t Dist = 0;
	int32_t dx = 0;
	int32_t dy = 0;

	while (true) {
		Tile* tile = map.getTile(x + dx, y + dy, z);
		if (tile && tile->getGround() && tile->getCreatureCount() == 0) {
			if (!tile->hasFlag(TILESTATE_IMMOVABLEBLOCKSOLID)) {
				if (!player) {
					if (!tile->hasFlag(TILESTATE_BLOCKPATH) && !tile->hasFlag(TILESTATE_BLOCKSOLID)) {
						if (creature && tile->queryAdd(INDEX_WHEREEVER, *creature, 1, FLAG_PATHFINDING) == RETURNVALUE_NOERROR) {
							if (tile->getHouse() != nullptr && allowHouses) {
								// We were able to spawn here
								break;
							}
						}
					}
				}
				else {
					break;
				}
			}
		}

		if (Direction == 2) {
			--dx;
			if (!(Dist + dx)) {
				Direction = 3;
			}
		}
		else if (Direction > 2) {
			if (Direction == 3) {
				if (++dy == Dist) {
					Direction = 4;
				}
			}
			else if (++dx == Dist + 1) {
				Direction = 1;
				Dist = dx;
			}
		}
		else {
			--dy;
			if (!(Dist + dy)) {
				Direction = 2;
			}
		}

		if (Dist > distance) {
			return false;
		}
	}

	x += dx;
	y += dy;
	return true;
}

bool Game::searchSpawnField(uint16_t& x, uint16_t& y, uint8_t& z, int32_t distance)
{
	distance = std::abs(distance);

	int32_t BestX = 0;
	int32_t BestY = 0;
	int32_t TieBreaker = -1;

	SpawnMatrix Map(-distance, distance, -distance, distance);
	for (int32_t xx = -distance; xx <= distance; xx++) {
		for (int32_t yy = -distance; yy <= distance; yy++) {
			int32_t dy = yy - Map.ymin;
			int32_t dx = xx - Map.xmin;
			Map.entry[Map.dx * dy + dx] = 0x7FFFFFFF;
		}
	}

	Map.entry[Map.dx * -Map.ymin - Map.xmin] = 0;

	for (int32_t Pass = 0;; Pass++) {
		bool Expanded = false;
		int32_t MinDistance = -distance;
		bool Found = false;
		for (Found = false; MinDistance <= distance; ++MinDistance) {
			for (int32_t j = -distance; j <= distance; ++j) {
				int32_t dy = j - Map.ymin;
				int32_t dx = MinDistance - Map.xmin;
				int32_t value = Map.entry[Map.dx * dy + dx];

				Tile* tile = map.getTile(MinDistance + x, j + y, z);
				if (value == Pass && tile && !tile->getHouse() && !tile->hasFlag(TILESTATE_PROTECTIONZONE)) {
					bool ExpansionPossible = true;
					bool LoginPossible = true;
					bool LoginBad = false;

					if (tile->hasFlag(TILESTATE_IMMOVABLEBLOCKSOLID)) {
						ExpansionPossible = false;
						LoginPossible = false;
					}
					else if (tile->hasFlag(TILESTATE_BLOCKSOLID)) {
						LoginBad = true;
					}

					if (tile->hasFlag(TILESTATE_BLOCKPATH)) {
						LoginBad = true;
					}

					if (tile->hasFlag(TILESTATE_IMMOVABLEBLOCKPATH)) {
						ExpansionPossible = false;
						LoginPossible = true;
					}

					if (ExpansionPossible || !Pass) {
						for (int32_t ddx = -1; ddx <= 1; ddx++) {
							for (int32_t ddy = -1; ddy <= 1; ddy++) {
								bool inRange = ddx + MinDistance >= -distance && ddx + MinDistance <= distance;
								if (inRange) {
									inRange = j + ddy >= -distance && j + ddy <= distance;
									if (inRange) {
										int32_t value = Map.entry[Map.dx * (j + ddy - Map.ymin) + (ddx + MinDistance - Map.xmin)];
										if (value > Pass) {
											Map.entry[Map.dx * (j + ddy - Map.ymin) + (ddx + MinDistance - Map.xmin)] = std::abs(ddy) + (Pass + std::abs(ddx));
										}
									}
								}
							}
						}
						Expanded = true;
					}

					if (LoginPossible) {
						int32_t rnd = uniform_random(0, 99);
						if (!LoginBad) {
							rnd = rnd + 100;
						}

						if (rnd > TieBreaker) {
							BestX = MinDistance;
							BestY = j;
							TieBreaker = rnd;
							Found = true;
						}
					}
				}
			}
		}

		if (Found && distance >= 0 || !Expanded) {
			break;
		}
	}

	if (TieBreaker < 0) {
		return false;
	}

	x += BestX;
	y += BestY;
	return true;
}

bool Game::searchSummonField(uint16_t& x, uint16_t& y, uint8_t& z, int32_t distance)
{
	int32_t NewTieBreaker = 0;
	int32_t TieBreaker = -1;
	int32_t BestX = 0;
	int32_t BestY = 0;

	for (int32_t dx = -distance; dx <= distance; ++dx) {
		for (int32_t dy = -distance; dy <= distance; ++dy) {
			NewTieBreaker = uniform_random(0, 99);
			if (NewTieBreaker > TieBreaker) {
				Tile* tile = map.getTile(x + dx, y + dy, z);
				if (tile && tile->getCreatureCount() == 0 && tile->getGround() && !tile->hasFlag(TILESTATE_BLOCKSOLID) && !tile->hasFlag(TILESTATE_BLOCKPATH)) {
					if (!tile->getHouse() && !tile->hasFlag(TILESTATE_PROTECTIONZONE)) {
						if (map.canThrowObjectTo(Position(x, y, z), Position(x + dx, y + dy, z))) {
							TieBreaker = NewTieBreaker;
							BestX = x + dx;
							BestY = y + dy;
						}
					}
				}
			}
		}
	}

	if (TieBreaker != -1) {
		x = BestX;
		y = BestY;
		return true;
	}

	return false;
}

void Game::playerAnswerModalWindow(uint32_t playerId, uint32_t modalWindowId, uint8_t button, uint8_t choice)
{
	Player* player = getPlayerByID(playerId);
	if (!player) {
		return;
	}

	if (!player->hasModalWindowOpen(modalWindowId)) {
		return;
	}

	player->onModalWindowHandled(modalWindowId);

	for (auto creatureEvent : player->getCreatureEvents(CREATURE_EVENT_MODALWINDOW)) {
		creatureEvent->executeModalWindow(player, modalWindowId, button, choice);
	}
}

void Game::addPlayer(Player* player)
{
	const std::string& lowercase_name = asLowerCaseString(player->getName());
	mappedPlayerNames[lowercase_name] = player;
	mappedPlayerGuids[player->getGUID()] = player;
	preloadedPlayerGuids[player->getGUID()] = player->getName();
	wildcardTree.insert(lowercase_name);
	players[player->getID()] = player;
}

void Game::removePlayer(Player* player)
{
	const std::string& lowercase_name = asLowerCaseString(player->getName());
	mappedPlayerNames.erase(lowercase_name);
	mappedPlayerGuids.erase(player->getGUID());
	wildcardTree.remove(lowercase_name);
	players.erase(player->getID());
}

void Game::addNpc(Npc* npc)
{
	npcs[npc->getID()] = npc;
}

void Game::removeNpc(Npc* npc)
{
	npcs.erase(npc->getID());
}

void Game::addMonster(Monster* monster)
{
	monsters[monster->getID()] = monster;
}

void Game::removeMonster(Monster* monster)
{
	monsters.erase(monster->getID());
}

Guild* Game::getGuild(uint32_t id) const
{
	auto it = guilds.find(id);
	if (it == guilds.end()) {
		return nullptr;
	}
	return it->second;
}

void Game::addGuild(Guild* guild)
{
	guilds[guild->getId()] = guild;
}

void Game::removeGuild(uint32_t guildId)
{
	guilds.erase(guildId);
}

void Game::internalRemoveItems(std::vector<Item*> itemList, uint32_t amount, bool stackable)
{
	if (stackable) {
		for (Item* item : itemList) {
			if (item->getItemCount() > amount) {
				internalRemoveItem(item, amount);
				break;
			} else {
				amount -= item->getItemCount();
				internalRemoveItem(item);
			}
		}
	} else {
		for (Item* item : itemList) {
			internalRemoveItem(item);
		}
	}
}

BedItem* Game::getBedBySleeper(uint32_t guid) const
{
	auto it = bedSleepersMap.find(guid);
	if (it == bedSleepersMap.end()) {
		return nullptr;
	}
	return it->second;
}

void Game::setBedSleeper(BedItem* bed, uint32_t guid)
{
	bedSleepersMap[guid] = bed;
}

void Game::removeBedSleeper(uint32_t guid)
{
	auto it = bedSleepersMap.find(guid);
	if (it != bedSleepersMap.end()) {
		bedSleepersMap.erase(it);
	}
}

Item* Game::getUniqueItem(uint16_t uniqueId)
{
	auto it = uniqueItems.find(uniqueId);
	if (it == uniqueItems.end()) {
		return nullptr;
	}
	return it->second;
}

bool Game::addUniqueItem(uint16_t uniqueId, Item* item)
{
	auto result = uniqueItems.emplace(uniqueId, item);
	if (!result.second) {
		std::cout << "Duplicate unique id: " << uniqueId << std::endl;
	}
	return result.second;
}

void Game::removeUniqueItem(uint16_t uniqueId)
{
	auto it = uniqueItems.find(uniqueId);
	if (it != uniqueItems.end()) {
		uniqueItems.erase(it);
	}
}

bool Game::reload(ReloadTypes_t reloadType)
{
	switch (reloadType) {
		case RELOAD_TYPE_CHAT: return g_chat->load();
		case RELOAD_TYPE_CONFIG: return g_config.reload();
		case RELOAD_TYPE_EVENTS: return g_events->load();
		case RELOAD_TYPE_ITEMS: return Item::items.reload();
		case RELOAD_TYPE_MONSTERS: return g_monsters.reload();
		case RELOAD_TYPE_NPCS: {
			Npcs::reload();
			return true;
		}

		case RELOAD_TYPE_RAIDS: return raids.reload() && raids.startup();

		case RELOAD_TYPE_SCRIPTS: {
			// commented out stuff is TODO, once we approach further in revscriptsys
			g_actions->clear();
			g_creatureEvents->clear();
			g_moveEvents->clear();
			g_talkActions->clear();
			g_globalEvents->clear();
			g_weapons->clear();
			g_weapons->loadDefaults();
			g_spells->clear();
			g_scripts->loadScripts("scripts", false, true);
			g_creatureEvents->removeInvalidEvents();
			/*
			Npcs::reload();
			raids.reload() && raids.startup();
			Item::items.reload();
			quests.reload();
			mounts.reload();
			g_config.reload();
			g_events->load();
			g_chat->load();
			*/
			return true;
		}

		default: {
			if (!g_monsters.reload()) {
				std::cout << "[Error - Game::reload] Failed to reload monsters." << std::endl;
				std::terminate();
			}

			g_config.reload();
			g_monsters.reload();
			Npcs::reload();
			raids.reload() && raids.startup();
			Item::items.reload();
			g_events->load();
			g_chat->load();
			g_actions->clear();
			g_creatureEvents->clear();
			g_moveEvents->clear();
			g_talkActions->clear();
			g_globalEvents->clear();
			g_spells->clear();
			g_scripts->loadScripts("scripts", false, true);
			g_creatureEvents->removeInvalidEvents();
			return true;
		}
	}
	return true;
}
