// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include "position.h"
#include "item.h"
#include "fileloader.h"

#include "tools.h"
#include "tile.h"
#include "town.h"
#include "house.h"
#include "spawn.h"

class Creature;
class Player;
class Game;
class Tile;
class Map;

static constexpr int32_t MAP_MAX_LAYERS = 16;

struct FindPathParams;
struct AStarNode
{
	AStarNode* parent;
	int_fast32_t f;
	uint16_t x, y;
};

static constexpr int32_t MAX_NODES = 512;

static constexpr int32_t MAP_NORMALWALKCOST = 10;
static constexpr int32_t MAP_DIAGONALWALKCOST = 25;

class AStarNodes
{
public:
	AStarNodes(uint32_t x, uint32_t y);

	AStarNode* createOpenNode(AStarNode* parent, uint32_t x, uint32_t y, int_fast32_t f);
	AStarNode* getBestNode();
	void closeNode(AStarNode* node);
	void openNode(AStarNode* node);
	int_fast32_t getClosedNodes() const;
	AStarNode* getNodeByPosition(uint32_t x, uint32_t y);

	static int_fast32_t getMapWalkCost(AStarNode* node, const Position& neighborPos);
	static int_fast32_t getTileWalkCost(Creature& creature, const Tile* tile);

private:
	AStarNode nodes[MAX_NODES];
	bool openNodes[MAX_NODES];
	std::unordered_map<uint32_t, AStarNode*> nodeTable;
	size_t curNode;
	int_fast32_t closedNodes;
};

static constexpr int32_t FLOOR_BITS = 3;
static constexpr int32_t FLOOR_SIZE = (1 << FLOOR_BITS);
static constexpr int32_t FLOOR_MASK = (FLOOR_SIZE - 1);

struct Floor
{
	constexpr Floor() = default;
	~Floor();

	// non-copyable
	Floor(const Floor&) = delete;
	Floor& operator=(const Floor&) = delete;

	Tile* tiles[FLOOR_SIZE][FLOOR_SIZE] = {};
};

class FrozenPathingConditionCall;
class QTreeLeafNode;

class QTreeNode
{
public:
	constexpr QTreeNode() = default;
	virtual ~QTreeNode();

	// non-copyable
	QTreeNode(const QTreeNode&) = delete;
	QTreeNode& operator=(const QTreeNode&) = delete;

	bool isLeaf() const { return leaf; }

	QTreeLeafNode* getLeaf(uint32_t x, uint32_t y);

	template <typename Leaf, typename Node>
	static Leaf getLeafStatic(Node node, uint32_t x, uint32_t y)
	{
		do {
			node = node->child[((x & 0x8000) >> 15) | ((y & 0x8000) >> 14)];
			if (!node) {
				return nullptr;
			}

			x <<= 1;
			y <<= 1;
		} while (!node->leaf);
		return static_cast<Leaf>(node);
	}

	QTreeLeafNode* createLeaf(uint32_t x, uint32_t y, uint32_t level);

protected:
	bool leaf = false;

private:
	QTreeNode* child[4] = {};

	friend class Map;
};

class QTreeLeafNode final : public QTreeNode
{
public:
	QTreeLeafNode()
	{
		leaf = true;
		newLeaf = true;
	}
	~QTreeLeafNode();

	// non-copyable
	QTreeLeafNode(const QTreeLeafNode&) = delete;
	QTreeLeafNode& operator=(const QTreeLeafNode&) = delete;

	Floor* createFloor(uint32_t z);
	Floor* getFloor(uint8_t z) const { return array[z]; }

	void addCreature(Creature* c);
	void removeCreature(Creature* c);

private:
	static bool newLeaf;
	QTreeLeafNode* leafS = nullptr;
	QTreeLeafNode* leafE = nullptr;
	Floor* array[MAP_MAX_LAYERS] = {};
	CreatureVector creature_list;
	CreatureVector player_list;

	friend class Map;
	friend class QTreeNode;
};

struct SpawnMatrix
{
	~SpawnMatrix() { delete[] entry; }

	SpawnMatrix(int32_t xmin, int32_t xmax, int32_t ymin, int32_t ymax)
	{
		dx = xmax - xmin + 1;
		this->xmin = xmin;
		dy = ymax - ymin + 1;
		this->ymin = ymin;

		entry = new int32_t[4 * dy * dx];
	}

	int32_t dx = 0;
	int32_t dy = 0;
	int32_t xmin = 0;
	int32_t ymin = 0;
	int32_t* entry = nullptr;
};

/**
 * Map class.
 * Holds all the actual map-data
 */

class Map
{
public:
	static constexpr int32_t maxViewportX = 11; // min value: maxClientViewportX + 1
	static constexpr int32_t maxViewportY = 11; // min value: maxClientViewportY + 1
	static constexpr int32_t maxClientViewportX = 8;
	static constexpr int32_t maxClientViewportY = 6;

	/**
	 * Load a map.
	 * \returns true if the map was loaded successfully
	 */
	bool loadMap(const std::string& identifier, bool loadHouses);
	bool loadMapPart(const std::string& identifier, bool loadSpawns, bool replaceTiles);

	uint32_t refreshMap();

	static bool save();

	/**
	 * Get a single tile.
	 * \returns A pointer to that tile.
	 */
	Tile* getTile(uint16_t x, uint16_t y, uint8_t z) const;
	Tile* getTile(const Position& pos) const { return getTile(pos.x, pos.y, pos.z); }

	/**
	 * Set a single tile.
	 */
	void setTile(uint16_t x, uint16_t y, uint8_t z, Tile* newTile, bool replaceExistingTiles = true);
	void setTile(const Position& pos, Tile* newTile, bool replaceExistingTiles = true)
	{
		setTile(pos.x, pos.y, pos.z, newTile, replaceExistingTiles);
	}

	/**
	 * Removes a single tile.
	 */
	void removeTile(uint16_t x, uint16_t y, uint8_t z);
	void removeTile(const Position& pos) { removeTile(pos.x, pos.y, pos.z); }

	/**
	 * Place a creature on the map
	 * \param centerPos The position to place the creature
	 * \param creature Creature to place on the map
	 * \param extendedPos If true, the creature will in first-hand be placed 2 tiles away
	 * \param forceLogin If true, placing the creature will not fail because of obstacles (creatures/chests)
	 */
	bool placeCreature(const Position& centerPos, Creature* creature, bool forceLogin = false);

	void moveCreature(Creature& creature, Tile& newTile, bool forceTeleport = false);

	void getSpectators(SpectatorVec& spectators, const Position& centerPos, bool multifloor = false,
	                   bool onlyPlayers = false, int32_t minRangeX = 0, int32_t maxRangeX = 0, int32_t minRangeY = 0,
	                   int32_t maxRangeY = 0);

	/**
	 * Checks if you can throw an object to that position
	 *	\param fromPos from Source point
	 *	\param toPos Destination point
	 *	\param rangex maximum allowed range horizontally
	 *	\param rangey maximum allowed range vertically
	 *	\param sameFloor checks if the destination is on same floor
	 *	\returns The result if you can throw there or not
	 */
	bool canThrowObjectTo(const Position& fromPos, const Position& toPos, bool multiFloor = false) const;

	const Tile* canWalkTo(const Creature& creature, const Position& pos) const;

	bool getPathMatching(Creature& creature, std::vector<Direction>& dirList,
	                     const FrozenPathingConditionCall& pathCondition, const FindPathParams& fpp) const;

	std::map<std::string, Position> waypoints;

	QTreeLeafNode* getQTNode(uint16_t x, uint16_t y)
	{
		return QTreeNode::getLeafStatic<QTreeLeafNode*, QTreeNode*>(&root, x, y);
	}

	Spawns spawns;
	Towns towns;
	Houses houses;

private:
	QTreeNode root;

	std::string spawnfile;
	std::string housefile;

	uint32_t width = 0;
	uint32_t height = 0;

	// Actually scans the map for spectators
	void getSpectatorsInternal(SpectatorVec& spectators, const Position& centerPos, int32_t minRangeX,
	                           int32_t maxRangeX, int32_t minRangeY, int32_t maxRangeY, int32_t minRangeZ,
	                           int32_t maxRangeZ, bool onlyPlayers) const;

	friend class Game;
	friend class IOMap;
};
