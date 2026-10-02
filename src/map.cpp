// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "iomap.h"
#include "combat.h"
#include "creature.h"
#include "game.h"
#include "monster.h"

#include <filesystem>

extern Game g_game;

bool Map::loadMap(const std::string& identifier, bool loadHouses)
{
	IOMap loader;

	MapDataLoadResult_t result = IOMap::loadMapData();

	if (result == MAP_DATA_LOAD_NONE) {
		if (!loader.loadMap(this, identifier, true)) {
			std::cout << "[Fatal - Map::loadMap] " << loader.getLastErrorString() << std::endl;
			return false;
		}
	} else if (result == MAP_DATA_LOAD_ERROR) {
		return false;
	}

	if (!IOMap::loadSpawns(this)) {
		std::cout << "[Warning - Map::loadMap] Failed to load spawn data." << std::endl;
	}

	if (loadHouses) {
		std::cout << "> Loading house..." << std::endl;
		if (!IOMap::loadHouses(this)) {
			std::cout << "[Warning - Map::loadMap] Failed to load house data." << std::endl;
		}

		std::cout << "> Loading house items..." << std::endl;
		if (!IOMap::loadHouseItems(this)) {
			return false;
		}

		std::cout << "> Loading house owners..." << std::endl;
		IOMap::loadHouseDatabaseInformation();
	}

	return true;
}

bool Map::loadMapPart(const std::string& identifier, bool loadSpawns, bool replaceTiles)
{
	IOMap loader;
	if (!loader.loadMap(this, identifier, replaceTiles)) {
		std::cout << "[Fatal - Map::loadMapPart] " << loader.getLastErrorString() << std::endl;
		return false;
	}

	if (loadSpawns && !IOMap::loadSpawns(this)) {
		std::cout << "[Warning - Map::loadMapPart] Failed to load spawn data." << std::endl;
		return false;
	}

	return true;
}

bool Map::save()
{
	bool saved = false;
	for (uint32_t tries = 0; tries < 3; tries++) { // wtf?
		if (IOMap::saveHouseDatabaseInformation()) {
			saved = true;
			break;
		}
	}

	if (!saved) {
		return false;
	}

	if (g_game.isMapSavingEnabled() && g_config.getBoolean(ConfigManager::ENABLE_MAP_DATA_FILES)) {
		IOMap::saveMapData();
	} else {
		std::cout << "> Live map data is not being saved." << std::endl;
	}

	return IOMap::saveHouseItems();
}

Tile* Map::getTile(uint16_t x, uint16_t y, uint8_t z) const
{
	if (z >= MAP_MAX_LAYERS) {
		return nullptr;
	}

	const QTreeLeafNode* leaf = QTreeNode::getLeafStatic<const QTreeLeafNode*, const QTreeNode*>(&root, x, y);
	if (!leaf) {
		return nullptr;
	}

	const Floor* floor = leaf->getFloor(z);
	if (!floor) {
		return nullptr;
	}
	return floor->tiles[x & FLOOR_MASK][y & FLOOR_MASK];
}

void Map::setTile(uint16_t x, uint16_t y, uint8_t z, Tile* newTile, bool replaceExistingTiles)
{
	if (z >= MAP_MAX_LAYERS) {
		std::cout << "ERROR: Attempt to set tile on invalid coordinate " << Position(x, y, z) << "!" << std::endl;
		return;
	}

	QTreeLeafNode::newLeaf = false;
	QTreeLeafNode* leaf = root.createLeaf(x, y, 15);

	if (QTreeLeafNode::newLeaf) {
		// update north
		QTreeLeafNode* northLeaf = root.getLeaf(x, y - FLOOR_SIZE);
		if (northLeaf) {
			northLeaf->leafS = leaf;
		}

		// update west leaf
		QTreeLeafNode* westLeaf = root.getLeaf(x - FLOOR_SIZE, y);
		if (westLeaf) {
			westLeaf->leafE = leaf;
		}

		// update south
		QTreeLeafNode* southLeaf = root.getLeaf(x, y + FLOOR_SIZE);
		if (southLeaf) {
			leaf->leafS = southLeaf;
		}

		// update east
		QTreeLeafNode* eastLeaf = root.getLeaf(x + FLOOR_SIZE, y);
		if (eastLeaf) {
			leaf->leafE = eastLeaf;
		}
	}

	Floor* floor = leaf->createFloor(z);
	uint32_t offsetX = x & FLOOR_MASK;
	uint32_t offsetY = y & FLOOR_MASK;

	Tile*& tile = floor->tiles[offsetX][offsetY];
	if (tile) {
		if (replaceExistingTiles) {
			tile->cleanItems();

			TileItemVector* items = newTile->getItemList();
			if (items) {
				for (auto it = items->rbegin(), end = items->rend(); it != end; ++it) {
					tile->addThing(*it);
				}
				items->clear();
			}

			Item* ground = newTile->getGround();
			if (ground) {
				tile->addThing(ground);
				newTile->setGround(nullptr);
			}

			tile->makeRefreshItemList();
		}

		delete newTile;
	} else {
		tile = newTile;

		if (tile->hasFlag(TILESTATE_REFRESH)) {
			g_game.addTileToRefresh(tile);
		}

		g_game.addTileToSave(tile);
	}
}

void Map::removeTile(uint16_t x, uint16_t y, uint8_t z)
{
	if (z >= MAP_MAX_LAYERS) {
		return;
	}

	const QTreeLeafNode* leaf = QTreeNode::getLeafStatic<const QTreeLeafNode*, const QTreeNode*>(&root, x, y);
	if (!leaf) {
		return;
	}

	const Floor* floor = leaf->getFloor(z);
	if (!floor) {
		return;
	}

	Tile* tile = floor->tiles[x & FLOOR_MASK][y & FLOOR_MASK];
	if (tile) {
		if (const CreatureVector* creatures = tile->getCreatures()) {
			for (int32_t i = creatures->size(); --i >= 0;) {
				if (Player* player = (*creatures)[i]->getPlayer()) {
					g_game.internalTeleport(player, player->getTown()->getTemplePosition(), false, FLAG_NOLIMIT);
				} else {
					g_game.removeCreature((*creatures)[i]);
				}
			}
		}

		if (TileItemVector* items = tile->getItemList()) {
			for (auto it = items->begin(), end = items->end(); it != end; ++it) {
				g_game.internalRemoveItem(*it);
			}
		}

		Item* ground = tile->getGround();
		if (ground) {
			g_game.internalRemoveItem(ground);
			tile->setGround(nullptr);
		}
	}
}

bool Map::placeCreature(const Position& centerPos, Creature* creature, bool forceLogin /* = false*/)
{
	House* toHouse = nullptr;
	if (Tile* toTile = getTile(centerPos)) {
		toHouse = toTile->getHouse();
	}
	Position targetPos = centerPos;

	int32_t index = 0;
	uint32_t flags = 0;
	Item* toItem = nullptr;

	if (!forceLogin) {
		bool result = g_game.searchLoginField(creature, targetPos.x, targetPos.y, targetPos.z, 1,
		                                      creature->getPlayer() != nullptr, toHouse != nullptr);

		if (result) {
			Tile* tile = getTile(targetPos);
			if (tile) {
				House* houseTile = tile->getHouse();
				if (houseTile && !toHouse) {
					result = false;

					if (Player* player = creature->getPlayer()) {
						if (player->hasFlag(PlayerFlags::PlayerFlag_CanEditHouses)) {
							result = true;
						}
					}
				} else {
					result = g_game.searchLoginField(creature, targetPos.x, targetPos.y, targetPos.z, 1,
					                                 creature->getPlayer() != nullptr, toHouse != nullptr);
				}
			}
		}

		// This has to be here, we like it or not
		if (!result) {
			if (Player* player = creature->getPlayer()) {
				targetPos = player->getTemplePosition();
				result = true;
			}
		}

		if (!result && creature->getPlayer() == nullptr) {
			return false;
		}
	}

	Tile* targetTile = getTile(targetPos);
	if (!targetTile) {
		return false;
	}

	Cylinder* toCylinder = targetTile->queryDestination(index, *creature, &toItem, flags);
	toCylinder->internalAddThing(creature);

	const Position& dest = toCylinder->getPosition();
	getQTNode(dest.x, dest.y)->addCreature(creature);
	return true;
}

void Map::moveCreature(Creature& creature, Tile& newTile, bool forceTeleport /* = false*/)
{
	Tile& oldTile = *creature.getTile();

	Position oldPos = oldTile.getPosition();
	Position newPos = newTile.getPosition();

	bool teleport = forceTeleport || !newTile.getGround() || !Position::areInRange<1, 1, 0>(oldPos, newPos);

	SpectatorVec spectators, newPosSpectators;
	getSpectators(spectators, oldPos, true);
	getSpectators(newPosSpectators, newPos, true);
	spectators.addSpectators(newPosSpectators);

	std::vector<int32_t> oldStackPosVector;
	for (Creature* spectator : spectators) {
		if (Player* tmpPlayer = spectator->getPlayer()) {
			if (tmpPlayer->canSeeCreature(&creature)) {
				oldStackPosVector.push_back(oldTile.getClientIndexOfCreature(tmpPlayer, &creature));
			} else {
				oldStackPosVector.push_back(-1);
			}
		}
	}

	// remove the creature
	oldTile.removeThing(&creature, 0);

	QTreeLeafNode* leaf = getQTNode(oldPos.x, oldPos.y);
	QTreeLeafNode* new_leaf = getQTNode(newPos.x, newPos.y);

	// Switch the node ownership
	if (leaf != new_leaf) {
		leaf->removeCreature(&creature);
		new_leaf->addCreature(&creature);
	}

	// add the creature
	newTile.addThing(&creature);

	if (!teleport) {
		if (oldPos.y > newPos.y) {
			creature.setDirection(DIRECTION_NORTH);
		} else if (oldPos.y < newPos.y) {
			creature.setDirection(DIRECTION_SOUTH);
		}

		if (oldPos.x < newPos.x) {
			creature.setDirection(DIRECTION_EAST);
		} else if (oldPos.x > newPos.x) {
			creature.setDirection(DIRECTION_WEST);
		}
	} else {
		if (newPos.x - oldPos.x <= 0) {
			if (newPos.x - oldPos.x < 0) {
				creature.setDirection(DIRECTION_WEST);
			} else if (newPos.y - oldPos.y < 0) {
				creature.setDirection(DIRECTION_NORTH);
			} else if (newPos.y - oldPos.y > 0) {
				creature.setDirection(DIRECTION_SOUTH);
			}
		} else {
			creature.setDirection(DIRECTION_EAST);
		}
	}

	// send to client
	size_t i = 0;
	for (Creature* spectator : spectators) {
		if (Player* tmpPlayer = spectator->getPlayer()) {
			// Use the correct stackpos
			int32_t stackpos = oldStackPosVector[i++];
			if (stackpos != -1) {
				tmpPlayer->sendMoveCreature(&creature, newPos, newTile.getClientIndexOfCreature(tmpPlayer, &creature),
				                            oldPos, stackpos, teleport);
			}
		}
	}

	// event method
	for (Creature* spectator : spectators) {
		spectator->onCreatureMove(&creature, &newTile, newPos, &oldTile, oldPos, teleport);
	}

	oldTile.postRemoveNotification(&creature, &newTile, 0);
	newTile.postAddNotification(&creature, &oldTile, 0);
}

void Map::getSpectatorsInternal(SpectatorVec& spectators, const Position& centerPos, int32_t minRangeX,
                                int32_t maxRangeX, int32_t minRangeY, int32_t maxRangeY, int32_t minRangeZ,
                                int32_t maxRangeZ, bool onlyPlayers) const
{
	int_fast16_t min_y = centerPos.y + minRangeY;
	int_fast16_t min_x = centerPos.x + minRangeX;
	int_fast16_t max_y = centerPos.y + maxRangeY;
	int_fast16_t max_x = centerPos.x + maxRangeX;

	int32_t minoffset = centerPos.getZ() - maxRangeZ;
	uint16_t x1 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (min_x + minoffset)));
	uint16_t y1 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (min_y + minoffset)));

	int32_t maxoffset = centerPos.getZ() - minRangeZ;
	uint16_t x2 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (max_x + maxoffset)));
	uint16_t y2 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (max_y + maxoffset)));

	int32_t startx1 = x1 - (x1 % FLOOR_SIZE);
	int32_t starty1 = y1 - (y1 % FLOOR_SIZE);
	int32_t endx2 = x2 - (x2 % FLOOR_SIZE);
	int32_t endy2 = y2 - (y2 % FLOOR_SIZE);

	const QTreeLeafNode* startLeaf =
	    QTreeNode::getLeafStatic<const QTreeLeafNode*, const QTreeNode*>(&root, startx1, starty1);
	const QTreeLeafNode* leafS = startLeaf;
	const QTreeLeafNode* leafE;

	for (int_fast32_t ny = starty1; ny <= endy2; ny += FLOOR_SIZE) {
		leafE = leafS;
		for (int_fast32_t nx = startx1; nx <= endx2; nx += FLOOR_SIZE) {
			if (leafE) {
				const CreatureVector& node_list = (onlyPlayers ? leafE->player_list : leafE->creature_list);
				for (Creature* creature : node_list) {
					const Position& cpos = creature->getPosition();
					if (minRangeZ > cpos.z || maxRangeZ < cpos.z) {
						continue;
					}

					int_fast16_t offsetZ = Position::getOffsetZ(centerPos, cpos);
					if ((min_y + offsetZ) > cpos.y || (max_y + offsetZ) < cpos.y || (min_x + offsetZ) > cpos.x ||
					    (max_x + offsetZ) < cpos.x) {
						continue;
					}

					spectators.emplace_back(creature);
				}
				leafE = leafE->leafE;
			} else {
				leafE = QTreeNode::getLeafStatic<const QTreeLeafNode*, const QTreeNode*>(&root, nx + FLOOR_SIZE, ny);
			}
		}

		if (leafS) {
			leafS = leafS->leafS;
		} else {
			leafS = QTreeNode::getLeafStatic<const QTreeLeafNode*, const QTreeNode*>(&root, startx1, ny + FLOOR_SIZE);
		}
	}
}

void Map::getSpectators(SpectatorVec& spectators, const Position& centerPos, bool multifloor /*= false*/,
                        bool onlyPlayers /*= false*/, int32_t minRangeX /*= 0*/, int32_t maxRangeX /*= 0*/,
                        int32_t minRangeY /*= 0*/, int32_t maxRangeY /*= 0*/)
{
	if (centerPos.z >= MAP_MAX_LAYERS) {
		return;
	}

	minRangeX = (minRangeX == 0 ? -maxViewportX : -minRangeX);
	maxRangeX = (maxRangeX == 0 ? maxViewportX : maxRangeX);
	minRangeY = (minRangeY == 0 ? -maxViewportY : -minRangeY);
	maxRangeY = (maxRangeY == 0 ? maxViewportY : maxRangeY);

	int32_t minRangeZ;
	int32_t maxRangeZ;

	if (multifloor) {
		if (centerPos.z > 7) {
			// underground (8->15)
			minRangeZ = std::max<int32_t>(centerPos.getZ() - 2, 0);
			maxRangeZ = std::min<int32_t>(centerPos.getZ() + 2, MAP_MAX_LAYERS - 1);
		} else if (centerPos.z == 6) {
			minRangeZ = 0;
			maxRangeZ = 8;
		} else if (centerPos.z == 7) {
			minRangeZ = 0;
			maxRangeZ = 9;
		} else {
			minRangeZ = 0;
			maxRangeZ = 7;
		}
	} else {
		minRangeZ = centerPos.z;
		maxRangeZ = centerPos.z;
	}

	getSpectatorsInternal(spectators, centerPos, minRangeX, maxRangeX, minRangeY, maxRangeY, minRangeZ, maxRangeZ,
	                      onlyPlayers);
}

bool Map::canThrowObjectTo(const Position& fromPos, const Position& toPos, bool multiFloor) const
{
	if (fromPos == toPos) {
		return true;
	}

	int32_t deltaz = Position::getDistanceZ(fromPos, toPos);
	if (deltaz > 2) {
		return false;
	}

	int32_t sx = fromPos.x;
	int32_t sy = fromPos.y;
	int32_t sz = fromPos.z;
	int32_t zx = toPos.x;
	int32_t zy = toPos.y;
	int32_t zz = toPos.z;

	int32_t sz_minus_one = sz - 1;
	int32_t sz_minus_power = sz - multiFloor;
	if (sz - multiFloor < 0) {
		sz_minus_power = 0;
	}

	if (sz_minus_one >= sz_minus_power) {
		while (true) {
			const Tile* tile = getTile(sx, sy, sz_minus_one);
			if (tile && tile->getGround()) {
				break;
			}

			if (--sz_minus_one < sz_minus_power) {
				break;
			}
		}
		sz_minus_power = sz_minus_one + 1;
	}

	int32_t to_zz_copy = zz;

	int32_t x_final_test = 0;
	int32_t y_final_test = 0;
	int32_t z_final_test = 0;

	if (sz_minus_power <= zz) {
		int32_t sz_minus_power_copy = sz_minus_power;
		int32_t zx_minus_sx = zx - sx;
		int32_t zy_minus_sy = zy - sy;

		while (true) {
			int32_t z_test = zz;
			if (to_zz_copy >= sz) {
				z_test = sz;
			}

			if (sz_minus_power_copy > z_test) {
				break;
			}

			if (zx != sx || zy != sy) {
				int32_t i = 1;

				int32_t abs_x = -zx_minus_sx;
				if (abs_x > -1) {
					abs_x = zx_minus_sx;
				}

				abs_x = std::abs(abs_x);

				int32_t abs_y = -zy_minus_sy;
				if (abs_y > -1) {
					abs_y = zy_minus_sy;
				}

				abs_y = std::abs(abs_y);

				int32_t delta = abs_y;
				if (abs_x >= abs_y) {
					delta = abs_x;
				}

				delta = std::abs(delta);

				if (i <= delta) {
					int32_t x_check = zx * i;
					int32_t y_check = zy * i;

					do {
						const Tile* tile = getTile((x_check + (delta - i) * sx) / delta,
						                           (y_check + (delta - i) * sy) / delta, sz_minus_power_copy);
						if (tile && tile->hasProperty(CONST_PROP_BLOCKPROJECTILE)) {
							break;
						}

						++i;
						x_check += zx;
						y_check += zy;
					} while (i <= delta);
				}

				int32_t new_delta = delta - i + 1;
				x_final_test = ((i - 1) * zx + new_delta * sx) / delta;
				to_zz_copy = zz;
				y_final_test = ((i - 1) * zy + new_delta * sy) / delta;
			} else {
				x_final_test = zx;
				y_final_test = zy;
			}

			z_final_test = sz_minus_power_copy;
			if (sz_minus_power_copy <= 14) {
				int32_t i = sz_minus_power_copy;

				if (sz_minus_power_copy < to_zz_copy) {
					while (true) {
						const Tile* tile = getTile(x_final_test, y_final_test, i);
						if (tile && tile->getGround()) {
							break;
						}

						to_zz_copy = zz;

						if (++i >= zz) {
							break;
						}
					}

					to_zz_copy = zz;
				}

				z_final_test = i;
			}

			if (x_final_test == zx && y_final_test == zy) {
				if (z_final_test == to_zz_copy) {
					return true;
				}
			}

			++sz_minus_power_copy;
		}
	}

	return false;
}

const Tile* Map::canWalkTo(const Creature& creature, const Position& pos) const
{
	Tile* tile = getTile(pos.x, pos.y, pos.z);
	if (creature.getTile() != tile) {
		uint32_t flags = FLAG_PATHFINDING;
		if (const Monster* monster = creature.getMonster()) {
			if (monster->getState() == STATE::PANIC) {
				flags |= FLAG_IGNOREFIELDDAMAGE;
			}
		} else {
			flags |= FLAG_IGNOREFIELDDAMAGE;
		}

		if (!tile || tile->queryAdd(0, creature, 1, flags) != RETURNVALUE_NOERROR) {
			return nullptr;
		}
	}
	return tile;
}

bool Map::getPathMatching(Creature& creature, std::vector<Direction>& dirList,
                          const FrozenPathingConditionCall& pathCondition, const FindPathParams& fpp) const
{
	Position pos = creature.getPosition();
	Position endPos;

	AStarNodes nodes(pos.x, pos.y);

	int32_t bestMatch = 0;

	static int_fast32_t dirNeighbors[8][5][2] = {
	    {{-1, 0}, {0, 1}, {1, 0}, {1, 1}, {-1, 1}},    {{-1, 0}, {0, 1}, {0, -1}, {-1, -1}, {-1, 1}},
	    {{-1, 0}, {1, 0}, {0, -1}, {-1, -1}, {1, -1}}, {{0, 1}, {1, 0}, {0, -1}, {1, -1}, {1, 1}},
	    {{1, 0}, {0, -1}, {-1, -1}, {1, -1}, {1, 1}},  {{-1, 0}, {0, -1}, {-1, -1}, {1, -1}, {-1, 1}},
	    {{0, 1}, {1, 0}, {1, -1}, {1, 1}, {-1, 1}},    {{-1, 0}, {0, 1}, {-1, -1}, {1, 1}, {-1, 1}}};
	static int_fast32_t allNeighbors[8][2] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}, {-1, -1}, {1, -1}, {1, 1}, {-1, 1}};

	const Position startPos = pos;

	AStarNode* found = nullptr;
	while (fpp.maxSearchDist != 0 || nodes.getClosedNodes() < 100) {
		AStarNode* n = nodes.getBestNode();
		if (!n) {
			if (found) {
				break;
			}
			return false;
		}

		const int_fast32_t x = n->x;
		const int_fast32_t y = n->y;
		pos.x = x;
		pos.y = y;
		if (pathCondition(startPos, pos, fpp, bestMatch)) {
			found = n;
			endPos = pos;
			if (bestMatch == 0) {
				break;
			}
		}

		uint_fast32_t dirCount;
		int_fast32_t* neighbors;
		if (n->parent) {
			const int_fast32_t offset_x = n->parent->x - x;
			const int_fast32_t offset_y = n->parent->y - y;
			if (offset_y == 0) {
				if (offset_x == -1) {
					neighbors = *dirNeighbors[DIRECTION_WEST];
				} else {
					neighbors = *dirNeighbors[DIRECTION_EAST];
				}
			} else if (!fpp.allowDiagonal || offset_x == 0) {
				if (offset_y == -1) {
					neighbors = *dirNeighbors[DIRECTION_NORTH];
				} else {
					neighbors = *dirNeighbors[DIRECTION_SOUTH];
				}
			} else if (offset_y == -1) {
				if (offset_x == -1) {
					neighbors = *dirNeighbors[DIRECTION_NORTHWEST];
				} else {
					neighbors = *dirNeighbors[DIRECTION_NORTHEAST];
				}
			} else if (offset_x == -1) {
				neighbors = *dirNeighbors[DIRECTION_SOUTHWEST];
			} else {
				neighbors = *dirNeighbors[DIRECTION_SOUTHEAST];
			}
			dirCount = fpp.allowDiagonal ? 5 : 3;
		} else {
			dirCount = 8;
			neighbors = *allNeighbors;
		}

		const int_fast32_t f = n->f;
		for (uint_fast32_t i = 0; i < dirCount; ++i) {
			pos.x = x + *neighbors++;
			pos.y = y + *neighbors++;

			if (fpp.maxSearchDist != 0 && (Position::getDistanceX(startPos, pos) > fpp.maxSearchDist ||
			                               Position::getDistanceY(startPos, pos) > fpp.maxSearchDist)) {
				continue;
			}

			if (fpp.keepDistance && !pathCondition.isInRange(startPos, pos, fpp)) {
				continue;
			}

			const Tile* tile;
			AStarNode* neighborNode = nodes.getNodeByPosition(pos.x, pos.y);
			if (neighborNode) {
				tile = getTile(pos.x, pos.y, pos.z);
			} else {
				tile = canWalkTo(creature, pos);
				if (!tile) {
					continue;
				}
			}

			// The cost (g) for this neighbor
			const int_fast32_t cost = AStarNodes::getMapWalkCost(n, pos);
			const int_fast32_t extraCost = AStarNodes::getTileWalkCost(creature, tile);
			const int_fast32_t newf = f + cost + extraCost;

			if (neighborNode) {
				if (neighborNode->f <= newf) {
					// The node on the closed/open list is cheaper than this one
					continue;
				}

				neighborNode->f = newf;
				neighborNode->parent = n;
				nodes.openNode(neighborNode);
			} else {
				// Does not exist in the open/closed list, create a new node
				neighborNode = nodes.createOpenNode(n, pos.x, pos.y, newf);
				if (!neighborNode) {
					if (found) {
						break;
					}
					return false;
				}
			}
		}

		nodes.closeNode(n);
	}

	if (!found) {
		return false;
	}

	int_fast32_t prevx = endPos.x;
	int_fast32_t prevy = endPos.y;
	int_fast32_t steps = 0;

	found = found->parent;
	while (found) {
		pos.x = found->x;
		pos.y = found->y;

		int_fast32_t dx = pos.getX() - prevx;
		int_fast32_t dy = pos.getY() - prevy;

		prevx = pos.x;
		prevy = pos.y;

		if (dx == 1 && dy == 1) {
			dirList.push_back(DIRECTION_NORTHWEST);
		} else if (dx == -1 && dy == 1) {
			dirList.push_back(DIRECTION_NORTHEAST);
		} else if (dx == 1 && dy == -1) {
			dirList.push_back(DIRECTION_SOUTHWEST);
		} else if (dx == -1 && dy == -1) {
			dirList.push_back(DIRECTION_SOUTHEAST);
		} else if (dx == 1) {
			dirList.push_back(DIRECTION_WEST);
		} else if (dx == -1) {
			dirList.push_back(DIRECTION_EAST);
		} else if (dy == 1) {
			dirList.push_back(DIRECTION_NORTH);
		} else if (dy == -1) {
			dirList.push_back(DIRECTION_SOUTH);
		}

		steps++;

		found = found->parent;
	}

	std::reverse(dirList.begin(), dirList.end());
	return true;
}

// AStarNodes

AStarNodes::AStarNodes(uint32_t x, uint32_t y) : nodes(), openNodes()
{
	curNode = 1;
	closedNodes = 0;
	openNodes[0] = true;

	AStarNode& startNode = nodes[0];
	startNode.parent = nullptr;
	startNode.x = x;
	startNode.y = y;
	startNode.f = 0;
	nodeTable[(x << 16) | y] = nodes;
}

AStarNode* AStarNodes::createOpenNode(AStarNode* parent, uint32_t x, uint32_t y, int_fast32_t f)
{
	if (curNode >= MAX_NODES) {
		return nullptr;
	}

	size_t retNode = curNode++;
	openNodes[retNode] = true;

	AStarNode* node = nodes + retNode;
	nodeTable[(x << 16) | y] = node;
	node->parent = parent;
	node->x = x;
	node->y = y;
	node->f = f;
	return node;
}

AStarNode* AStarNodes::getBestNode()
{
	if (curNode == 0) {
		return nullptr;
	}

	int32_t best_node_f = std::numeric_limits<int32_t>::max();
	int32_t best_node = -1;
	for (size_t i = 0; i < curNode; i++) {
		if (openNodes[i] && nodes[i].f < best_node_f) {
			best_node_f = nodes[i].f;
			best_node = i;
		}
	}

	if (best_node >= 0) {
		return nodes + best_node;
	}
	return nullptr;
}

void AStarNodes::closeNode(AStarNode* node)
{
	size_t index = node - nodes;
	assert(index < MAX_NODES);
	openNodes[index] = false;
	++closedNodes;
}

void AStarNodes::openNode(AStarNode* node)
{
	size_t index = node - nodes;
	assert(index < MAX_NODES);
	if (!openNodes[index]) {
		openNodes[index] = true;
		--closedNodes;
	}
}

int_fast32_t AStarNodes::getClosedNodes() const { return closedNodes; }

AStarNode* AStarNodes::getNodeByPosition(uint32_t x, uint32_t y)
{
	auto it = nodeTable.find((x << 16) | y);
	if (it == nodeTable.end()) {
		return nullptr;
	}
	return it->second;
}

int_fast32_t AStarNodes::getMapWalkCost(AStarNode* node, const Position& neighborPos)
{
	if (std::abs(node->x - neighborPos.x) == std::abs(node->y - neighborPos.y)) {
		// diagonal movement extra cost
		return MAP_DIAGONALWALKCOST;
	}
	return MAP_NORMALWALKCOST;
}

int_fast32_t AStarNodes::getTileWalkCost(Creature& creature, const Tile* tile)
{
	int_fast32_t cost = 0;
	if (Creature* topCreature = tile->getTopVisibleCreature(&creature)) {
		// destroy creature cost
		bool destroyCreatureCost = true;
		if (const Monster* monster = creature.getMonster()) {
			if (monster->canPushCreatures() || monster->isPathBlockingChecking()) {
				destroyCreatureCost = false;
			}
		}

		if (destroyCreatureCost) {
			cost += MAP_NORMALWALKCOST * 3;
		}
	}

	if (const MagicField* field = tile->getFieldItem()) {
		CombatType_t combatType = field->getCombatType();
		const Monster* monster = creature.getMonster();
		if (!creature.isImmune(combatType) && !creature.hasCondition(Combat::DamageToConditionType(combatType)) &&
		    (monster && !monster->canWalkOnFieldType(combatType))) {
			cost += MAP_NORMALWALKCOST * 18;
		}
	}

	return cost;
}

// Floor
Floor::~Floor()
{
	for (auto& row : tiles) {
		for (auto tile : row) {
			delete tile;
		}
	}
}

// QTreeNode
QTreeNode::~QTreeNode()
{
	for (auto* ptr : child) {
		delete ptr;
	}
}

QTreeLeafNode* QTreeNode::getLeaf(uint32_t x, uint32_t y)
{
	if (leaf) {
		return static_cast<QTreeLeafNode*>(this);
	}

	QTreeNode* node = child[((x & 0x8000) >> 15) | ((y & 0x8000) >> 14)];
	if (!node) {
		return nullptr;
	}
	return node->getLeaf(x << 1, y << 1);
}

QTreeLeafNode* QTreeNode::createLeaf(uint32_t x, uint32_t y, uint32_t level)
{
	if (!isLeaf()) {
		uint32_t index = ((x & 0x8000) >> 15) | ((y & 0x8000) >> 14);
		if (!child[index]) {
			if (level != FLOOR_BITS) {
				child[index] = new QTreeNode();
			} else {
				child[index] = new QTreeLeafNode();
				QTreeLeafNode::newLeaf = true;
			}
		}
		return child[index]->createLeaf(x * 2, y * 2, level - 1);
	}
	return static_cast<QTreeLeafNode*>(this);
}

// QTreeLeafNode
bool QTreeLeafNode::newLeaf = false;

QTreeLeafNode::~QTreeLeafNode()
{
	for (auto* ptr : array) {
		delete ptr;
	}
}

Floor* QTreeLeafNode::createFloor(uint32_t z)
{
	if (!array[z]) {
		array[z] = new Floor();
	}
	return array[z];
}

void QTreeLeafNode::addCreature(Creature* c)
{
	creature_list.push_back(c);

	if (c->getPlayer()) {
		player_list.push_back(c);
	}
}

void QTreeLeafNode::removeCreature(Creature* c)
{
	auto iter = std::find(creature_list.begin(), creature_list.end(), c);
	assert(iter != creature_list.end());
	*iter = creature_list.back();
	creature_list.pop_back();

	if (c->getPlayer()) {
		iter = std::find(player_list.begin(), player_list.end(), c);
		assert(iter != player_list.end());
		*iter = player_list.back();
		player_list.pop_back();
	}
}

uint32_t Map::refreshMap()
{
	uint64_t start = OTSYS_TIME();
	size_t tiles = 0;

	for (Tile* tile : g_game.getTilesToRefresh()) {
		if (!tile) {
			continue;
		}

		++tiles;
		tile->refresh();
	}

	std::cout << "> Map Refreshed " << tiles << " tiles in " << (OTSYS_TIME() - start) / (1000.) << " seconds."
	          << std::endl;
	return tiles;
}
