// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "iomap.h"

#include "bed.h"
#include "game.h"
#include "scriptwriter.h"

#include <fmt/format.h>
#include <filesystem>
#include <ranges>

Tile* IOMap::createTile(Item*& ground, uint16_t x, uint16_t y, uint8_t z)
{
	Tile* tile = new Tile(x, y, z);
	if (ground) {
		tile->internalAddThing(ground);
		ground->startDecaying();
		ground = nullptr;
	}
	return tile;
}

bool IOMap::loadMap(Map* map, const std::string& fileName, bool replaceExistingTiles)
{
	std::cout << "> Loading " << fileName << std::endl;

	int64_t start = OTSYS_TIME();
	try {
		OTB::Loader loader{fileName, OTB::Identifier{{'O', 'T', 'B', 'M'}}};
		auto& root = loader.parseTree();

		PropStream propStream;
		if (!loader.getProps(root, propStream)) {
			setLastErrorString("Could not read root property.");
			return false;
		}

		OTBM_root_header root_header;
		if (!propStream.read(root_header)) {
			setLastErrorString("Could not read header.");
			return false;
		}

		uint32_t headerVersion = root_header.version;
		if (headerVersion == 0) {
			// In otbm version 1 the count variable after splashes/fluidcontainers and stackables
			// are saved as attributes instead, this solves a lot of problems with items
			// that are changed (stackable/charges/fluidcontainer/splash) during an update.
			setLastErrorString(
			    "This map need to be upgraded by using the latest map editor version to be able to load correctly.");
			return false;
		}

		if (headerVersion > 2) {
			setLastErrorString("Unknown OTBM version detected.");
			return false;
		}

		std::cout << "> Map size:" << root_header.width << "x" << root_header.height << '.' << std::endl;

		map->width = root_header.width;
		map->height = root_header.height;

		if (root.children.size() != 1 || root.children[0].type != OTBM_MAP_DATA) {
			setLastErrorString("Could not read data node.");
			return false;
		}

		auto& mapNode = root.children[0];
		if (!parseMapDataAttributes(loader, mapNode, *map, fileName)) {
			return false;
		}

		for (auto& mapDataNode : mapNode.children) {
			if (mapDataNode.type == OTBM_TILE_AREA) {
				if (!parseTileArea(loader, mapDataNode, *map, replaceExistingTiles)) {
					return false;
				}
			} else if (mapDataNode.type == OTBM_TOWNS) {
				if (!parseTowns(loader, mapDataNode, *map)) {
					return false;
				}
			} else if (mapDataNode.type == OTBM_WAYPOINTS && headerVersion > 1) {
				if (!parseWaypoints(loader, mapDataNode, *map)) {
					return false;
				}
			} else {
				setLastErrorString("Unknown map node.");
				return false;
			}
		}
	} catch (const OTB::InvalidOTBFormat& err) {
		setLastErrorString(err.what());
		return false;
	}

	std::cout << "> Map loading time: " << (OTSYS_TIME() - start) / (1000.) << " seconds." << std::endl;
	return true;
}

MapDataLoadResult_t IOMap::loadMapData()
{
	if (!g_config.getBoolean(ConfigManager::ENABLE_MAP_DATA_FILES)) {
		return MAP_DATA_LOAD_NONE;
	}

	// compare date times
	auto otbmLastWriteTime = std::filesystem::last_write_time(
	    fmt::format("data/world/{:s}.otbm", g_config.getString(ConfigManager::MAP_NAME)));

	const std::string filename = "gamedata/map.tvpm";
	std::ifstream fileTest(filename, std::ios::binary);
	if (fileTest.is_open()) {
		auto liveMapDataWriteTime = std::filesystem::last_write_time("gamedata/map.tvpm");

		if (otbmLastWriteTime > liveMapDataWriteTime) {
			std::cout << "> INFO: Original OTBM map is newer than live map data, proceeding to load original OTBM map."
			          << std::endl;
			g_game.toggleSendPlayersToTemple(true);
			return MAP_DATA_LOAD_NONE;
		} else {
			std::cout << "> INFO: Live Map Data is being used." << std::endl;
		}

		int64_t start = OTSYS_TIME();

		g_game.map.width = g_game.map.height = 65000; // default map size is max size

		const auto& size = std::filesystem::file_size(std::filesystem::path(filename));

		std::string content(size, '\0');
		fileTest.read(content.data(), size);

		PropStream propStream;
		propStream.init(content.data(), size);

		uint64_t totalTiles = 0;
		propStream.read<uint64_t>(totalTiles);

		for (uint64_t i = 0; i < totalTiles; i++) {
			uint32_t houseId;
			uint16_t x, y;
			uint8_t z;

			propStream.read<uint16_t>(x);
			propStream.read<uint16_t>(y);
			propStream.read<uint8_t>(z);
			propStream.read<uint32_t>(houseId);

			Tile* tile = new Tile(x, y, z);
			if (houseId != 0) {
				House* house = g_game.map.houses.addHouse(houseId);
				tile->setHouse(house);
			}

			uint32_t tileFlags = 0;
			propStream.read<uint32_t>(tileFlags);
			tile->setFlags(tileFlags);

			uint32_t totalItems = 0;
			propStream.read<uint32_t>(totalItems);

			if (totalItems == 0) {
				g_game.map.setTile(tile->getPosition(), tile);
				continue;
			}

			for (uint32_t ii = 0; ii < totalItems; ii++) {
				Item* item = Item::CreateItem(propStream);
				if (!item) {
					std::cout << fmt::format("ERROR - [IOMapSerialize::loadMapData]: Failed to create item - {:s}",
					                         filename)
					          << std::endl;
					delete tile;
					return MAP_DATA_LOAD_ERROR;
				}

				if (!item->unserializeTVPFormat(propStream)) {
					std::cout << "> ERROR - [IOMapSerialize::loadMapData]: Failed to unserialize item in file "
					          << filename << std::endl;
					delete item;
					delete tile;
					return MAP_DATA_LOAD_ERROR;
				}

				tile->internalAddThing(item);
				item->startDecaying();
			}

			tile->makeRefreshItemList();
			g_game.map.setTile(tile->getPosition(), tile);
		}

		uint8_t totalTowns = 0;
		propStream.read<uint8_t>(totalTowns);

		for (uint32_t i = 0; i < totalTowns; i++) {
			uint8_t id;
			propStream.read<uint8_t>(id);
			std::string name;
			propStream.readString(name);
			uint32_t x, y;
			uint8_t z;
			propStream.read<uint32_t>(x);
			propStream.read<uint32_t>(y);
			propStream.read<uint8_t>(z);
			Town* town = new Town(id);
			town->setName(name);
			town->setTemplePos(Position(x, y, z));
			g_game.map.towns.addTown(id, town);
		}

		uint32_t totalHouses = 0;
		propStream.read<uint32_t>(totalHouses);

		for (uint32_t i = 0; i < totalHouses; i++) {
			uint32_t houseId;
			propStream.read<uint32_t>(houseId);

			std::string name;
			propStream.readString(name);

			uint32_t townId;
			propStream.read<uint32_t>(townId);

			uint32_t rent;
			propStream.read<uint32_t>(rent);

			Position entryPos;
			propStream.read<uint16_t>(entryPos.x);
			propStream.read<uint16_t>(entryPos.y);
			propStream.read<uint8_t>(entryPos.z);

			House* house = g_game.map.houses.addHouse(houseId);
			house->setName(name);
			house->setTownId(townId);
			house->setRent(rent);
			house->setEntryPos(entryPos);
		}

		propStream.readString(g_game.map.spawnfile);
		propStream.readString(g_game.map.housefile);

		std::cout << "> Live Map loading time: " << (OTSYS_TIME() - start) / (1000.) << " seconds." << std::endl;
	} else {
		return MAP_DATA_LOAD_NONE;
	}

	g_game.cleanup();
	return MAP_DATA_LOAD_FOUND;
}

bool IOMap::loadHouseItems(Map* map)
{
	const int64_t start = OTSYS_TIME();

	for (const auto& it : g_game.map.houses.getHouses()) {
		House* house = it.second;

		const std::string filename = fmt::format("gamedata/houses/{:d}.tvph", house->getId());
		if (!std::filesystem::exists(filename)) {
			continue;
		}

		if (!loadHouseData(house, filename)) {
			std::cout << "ERROR: Could not load house data-file: " << house->getId() << std::endl;
			return false;
		}
	}

	std::cout << "Loaded house items in: " << (OTSYS_TIME() - start) / (1000.) << " s" << std::endl;
	return true;
}

bool IOMap::loadHouseData(House* house, const std::string_view& fileName)
{
	ScriptReader script;
	if (!script.loadScript(fileName)) {
		return false;
	}

	while (script.canRead()) {
		script.nextToken();
		if (script.getToken() == TOKEN_ENDOFFILE) {
			break;
		}

		if (script.getToken() != TOKEN_SPECIAL || script.getSpecial() != '[') {
			script.error("position expected");
			return false;
		}

		const uint16_t x = script.readNumber<uint16_t>();
		script.readSymbol(',');
		const uint16_t y = script.readNumber<uint16_t>();
		script.readSymbol(',');
		const uint8_t z = script.readNumber<uint8_t>();
		script.readSymbol(']');
		script.readSymbol(':');
		script.readSymbol('{');

		Tile* tile = g_game.map.getTile(x, y, z);
		if (!tile) {
			script.error("tile no longer exists");
			return false;
		}

		bool loadedHouse = true;

		std::vector<Item*> preloadedItems{};

		script.nextToken();
		while (script.canRead()) {
			if (script.getToken() == TOKEN_NUMBER) {
				Item* item = Item::CreateItem(script);
				if (!item) {
					script.error("failed to create item");
					loadedHouse = false;
					break;
				}

				if (!item->unserializeTVPFormat(script)) {
					script.error("failed to load item data");
					delete item;
					loadedHouse = false;
					break;
				}

				if (!item->isHouseItem()) {
					script.error(fmt::format("item {:d} is not a house item", item->getID()));
					return false;
				}

				preloadedItems.push_back(item);
			} else if (script.getSpecial() == ',') {
				script.nextToken();
			} else if (script.getSpecial() == '}') {
				break;
			} else {
				script.error("expected tile data");
				return false;
			}
		}

		if (loadedHouse) {
			// house tile is saved on disk, so clean it up no matter what
			// also cleans up the stock furniture from CIP map that comes from original OTBM
			tile->cleanHouseItems();

			for (Item* houseItem : preloadedItems) {
				tile->internalAddThing(houseItem);
				houseItem->startDecaying();
			}
		} else {
			for (const Item* houseItem : preloadedItems) {
				delete houseItem;
			}
		}
	}

	house->updateDoorDescription();
	return true;
}

bool IOMap::saveMapData()
{
	if (!g_config.getBoolean(ConfigManager::ENABLE_MAP_DATA_FILES)) {
		return true;
	}

	std::cout << "> Saving map data..." << std::endl;

	int64_t start = OTSYS_TIME();

	std::ostringstream ss;
	ss << "gamedata/map.tvpm";

	std::ofstream file;
	file.open(ss.str(), std::ios::out | std::ios::binary | std::ios::trunc);
	if (!file.is_open()) {
		std::cout << "> ERROR: Cannot open " << ss.str() << " for saving." << std::endl;
		return false;
	}

	PropWriteStream f;

	const auto& tiles = g_game.getTilesToSave();

	f.write<uint64_t>(tiles.size());

	for (const Tile* tile : tiles) {
		const Position& pos = tile->getPosition();

		f.write<uint16_t>(pos.x);
		f.write<uint16_t>(pos.y);
		f.write<uint8_t>(pos.z);

		if (const House* house = tile->getHouse()) {
			f.write<uint32_t>(house->getId());
		} else {
			f.write<uint32_t>(0);
		}

		std::vector<const Item*> savingItems;

		if (const Item* ground = tile->getGround()) {
			savingItems.push_back(ground);
		}

		std::list<Item*> borderItems;
		if (const auto& items = tile->getItemList()) {
			for (auto it = items->rbegin(); it != items->rend(); it++) {
				Item* item = (*it);
				if (item->isAlwaysOnTop() && Item::items[item->getID()].alwaysOnTopOrder == 1) {
					borderItems.push_front(item);
				} else {
					savingItems.push_back(item);
				}
			}
		}

		uint32_t realTileFlags = tile->getFlags();
		realTileFlags &= ~TILESTATE_FLOORCHANGE;

		f.write<uint32_t>(realTileFlags);

		f.write<uint32_t>(savingItems.size() + borderItems.size());

		for (const Item* item : borderItems) {
			item->serializeTVPFormat(f);
		}

		for (const Item* item : savingItems) {
			item->serializeTVPFormat(f);
		}
	}

	f.write<uint8_t>(g_game.map.towns.getTowns().size());
	for (const auto& it : g_game.map.towns.getTowns()) {
		Town* town = it.second;
		f.write<uint8_t>(town->getID());
		f.writeString(town->getName());
		f.write<uint32_t>(town->getTemplePosition().x);
		f.write<uint32_t>(town->getTemplePosition().y);
		f.write<uint8_t>(town->getTemplePosition().z);
	}

	f.write<uint32_t>(g_game.map.houses.getHouses().size());
	for (const auto& it : g_game.map.houses.getHouses()) {
		House* house = it.second;
		f.write<uint32_t>(house->getId());
		f.writeString(house->getName());
		f.write<uint32_t>(house->getTownId());
		f.write<uint32_t>(house->getRent());
		f.write<uint16_t>(house->getEntryPosition().x);
		f.write<uint16_t>(house->getEntryPosition().y);
		f.write<uint8_t>(house->getEntryPosition().z);
	}

	f.writeString(g_game.map.spawnfile);
	f.writeString(g_game.map.housefile);

	size_t size;
	const char* data = f.getStream(size);
	file.write(data, size);
	file.close();

	std::cout << "> Saved map data in: " << (OTSYS_TIME() - start) / (1000.) << " s" << std::endl;
	return true;
}

bool IOMap::saveHouseItems()
{
	std::cout << "> Saving house items..." << std::endl;
	int64_t start = OTSYS_TIME();

	for (const auto& it : g_game.map.houses.getHouses()) {
		const House* house = it.second;
		if (!saveHouseTVPFormat(house)) {
			std::cout << "> ERROR: Failed to save house " << house->getId() << ":" << house->getName() << std::endl;
			return false;
		}
	}

	std::cout << "> Saved house data files in: " << (OTSYS_TIME() - start) / (1000.) << " s" << std::endl;
	return true;
}

bool IOMap::loadHouseDatabaseInformation()
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery("SELECT `id`, `owner`, `paid`, `warnings` FROM `houses`");
	if (!result) {
		return false;
	}

	do {
		House* house = g_game.map.houses.getHouse(result->getNumber<uint32_t>("id"));
		if (house) {
			house->setOwner(result->getNumber<uint32_t>("owner"), false);
			house->setPaidUntil(result->getNumber<time_t>("paid"));
			house->setPayRentWarnings(result->getNumber<uint32_t>("warnings"));
		}
	} while (result->next());

	result = db.storeQuery("SELECT `house_id`, `listid`, `list` FROM `house_lists`");
	if (result) {
		do {
			House* house = g_game.map.houses.getHouse(result->getNumber<uint32_t>("house_id"));
			if (house) {
				house->setAccessList(result->getNumber<uint32_t>("listid"), result->getString("list"));
			}
		} while (result->next());
	}
	return true;
}

bool IOMap::saveHouseDatabaseInformation()
{
	Database& db = Database::getInstance();

	DBTransaction transaction;
	if (!transaction.begin()) {
		return false;
	}

	if (!db.executeQuery("DELETE FROM `house_lists`")) {
		return false;
	}

	for (const auto& it : g_game.map.houses.getHouses()) {
		House* house = it.second;
		DBResult_ptr result = db.storeQuery(fmt::format("SELECT `id` FROM `houses` WHERE `id` = {:d}", house->getId()));
		if (result) {
			db.executeQuery(fmt::format(
			    "UPDATE `houses` SET `owner` = {:d}, `paid` = {:d}, `warnings` = {:d}, `name` = {:s}, `town_id` = {:d}, `rent` = {:d}, `size` = {:d}, `beds` = {:d} WHERE `id` = {:d}",
			    house->getOwner(), house->getPaidUntil(), house->getPayRentWarnings(),
			    db.escapeString(house->getName()), house->getTownId(), house->getRent(), house->getTiles().size(),
			    house->getBedCount(), house->getId()));
		} else {
			db.executeQuery(fmt::format(
			    "INSERT INTO `houses` (`id`, `owner`, `paid`, `warnings`, `name`, `town_id`, `rent`, `size`, `beds`) VALUES ({:d}, {:d}, {:d}, {:d}, {:s}, {:d}, {:d}, {:d}, {:d})",
			    house->getId(), house->getOwner(), house->getPaidUntil(), house->getPayRentWarnings(),
			    db.escapeString(house->getName()), house->getTownId(), house->getRent(), house->getTiles().size(),
			    house->getBedCount()));
		}
	}

	DBInsert stmt("INSERT INTO `house_lists` (`house_id` , `listid` , `list`) VALUES ");

	for (const auto& it : g_game.map.houses.getHouses()) {
		House* house = it.second;

		std::string listText;
		if (house->getAccessList(GUEST_LIST, listText) && !listText.empty()) {
			if (!stmt.addRow(fmt::format("{:d}, {}, {:s}", house->getId(), tvp::to_underlying(GUEST_LIST),
			                             db.escapeString(listText)))) {
				return false;
			}

			listText.clear();
		}

		if (house->getAccessList(SUBOWNER_LIST, listText) && !listText.empty()) {
			if (!stmt.addRow(fmt::format("{:d}, {}, {:s}", house->getId(), tvp::to_underlying(SUBOWNER_LIST),
			                             db.escapeString(listText)))) {
				return false;
			}

			listText.clear();
		}

		for (Door* door : house->getDoors()) {
			if (door->getAccessList(listText) && !listText.empty()) {
				if (!stmt.addRow(fmt::format("{:d}, {:d}, {:s}", house->getId(), door->getDoorId(),
				                             db.escapeString(listText)))) {
					return false;
				}

				listText.clear();
			}
		}
	}

	if (!stmt.execute()) {
		return false;
	}

	return transaction.commit();
}

bool IOMap::saveHouseTVPFormat(const House* house)
{
	std::ostringstream ss;
	ss << "gamedata/houses/" << house->getId() << ".tvph";

	ScriptWriter script;
	if (!script.open(ss.str())) {
		std::cout << "> ERROR: Cannot open " << ss.str() << " for saving." << std::endl;
		return false;
	}

	script.writeLine(fmt::format("# House data-file: {:d}-{:s}", house->getId(), house->getName()));
	script.writeLine();

	for (Tile* tile : house->getTiles()) {
		script.writePosition(tile->getPosition());
		script.writeText(": ");
		script.writeText("{");

		if (const auto& items = tile->getItemList()) {
			std::vector<Item*> houseItems;
			for (auto item : std::ranges::reverse_view(*items)) {
				if (!item->isHouseItem()) {
					continue;
				}

				houseItems.push_back(item);
			}

			for (auto it = houseItems.begin(); it != houseItems.end();) {
				Item* item = *it;
				item->serializeTVPFormat(script);
				if (++it != houseItems.end()) {
					script.writeText(", ");
				}
			}
		}

		// End of tile items
		script.writeText("}");
		script.writeLine();
	}

	script.close();
	return true;
}

bool IOMap::parseMapDataAttributes(OTB::Loader& loader, const OTB::Node& mapNode, Map& map, const std::string& fileName)
{
	PropStream propStream;
	if (!loader.getProps(mapNode, propStream)) {
		setLastErrorString("Could not read map data attributes.");
		return false;
	}

	std::string mapDescription;
	std::string tmp;

	uint8_t attribute;
	while (propStream.read<uint8_t>(attribute)) {
		switch (attribute) {
			case OTBM_ATTR_DESCRIPTION:
				if (!propStream.readString(mapDescription)) {
					setLastErrorString("Invalid description tag.");
					return false;
				}
				break;

			case OTBM_ATTR_EXT_SPAWN_FILE:
				if (!propStream.readString(tmp)) {
					setLastErrorString("Invalid spawn tag.");
					return false;
				}

				map.spawnfile = fileName.substr(0, fileName.rfind('/') + 1);
				map.spawnfile += tmp;
				break;

			case OTBM_ATTR_EXT_HOUSE_FILE:
				if (!propStream.readString(tmp)) {
					setLastErrorString("Invalid house tag.");
					return false;
				}

				map.housefile = fileName.substr(0, fileName.rfind('/') + 1);
				map.housefile += tmp;
				break;

			default:
				setLastErrorString("Unknown header node.");
				return false;
		}
	}
	return true;
}

bool IOMap::parseWaypoints(OTB::Loader& loader, const OTB::Node& waypointsNode, Map& map)
{
	PropStream propStream;
	for (auto& node : waypointsNode.children) {
		if (node.type != OTBM_WAYPOINT) {
			setLastErrorString("Unknown waypoint node.");
			return false;
		}

		if (!loader.getProps(node, propStream)) {
			setLastErrorString("Could not read waypoint data.");
			return false;
		}

		std::string name;
		if (!propStream.readString(name)) {
			setLastErrorString("Could not read waypoint name.");
			return false;
		}

		OTBM_Destination_coords waypoint_coords;
		if (!propStream.read(waypoint_coords)) {
			setLastErrorString("Could not read waypoint coordinates.");
			return false;
		}

		map.waypoints[name] = Position(waypoint_coords.x, waypoint_coords.y, waypoint_coords.z);
	}
	return true;
}

bool IOMap::parseTowns(OTB::Loader& loader, const OTB::Node& townsNode, Map& map)
{
	for (auto& townNode : townsNode.children) {
		PropStream propStream;
		if (townNode.type != OTBM_TOWN) {
			setLastErrorString("Unknown town node.");
			return false;
		}

		if (!loader.getProps(townNode, propStream)) {
			setLastErrorString("Could not read town data.");
			return false;
		}

		uint32_t townId;
		if (!propStream.read<uint32_t>(townId)) {
			setLastErrorString("Could not read town id.");
			return false;
		}

		Town* town = map.towns.getTown(townId);
		if (!town) {
			town = new Town(townId);
			map.towns.addTown(townId, town);
		}

		std::string townName;
		if (!propStream.readString(townName)) {
			setLastErrorString("Could not read town name.");
			return false;
		}

		town->setName(townName);

		OTBM_Destination_coords town_coords;
		if (!propStream.read(town_coords)) {
			setLastErrorString("Could not read town coordinates.");
			return false;
		}

		town->setTemplePos(Position(town_coords.x, town_coords.y, town_coords.z));
	}
	return true;
}

bool IOMap::parseTileArea(OTB::Loader& loader, const OTB::Node& tileAreaNode, Map& map, bool replaceExistingTiles)
{
	PropStream propStream;
	if (!loader.getProps(tileAreaNode, propStream)) {
		setLastErrorString("Invalid map node.");
		return false;
	}

	OTBM_Destination_coords area_coord;
	if (!propStream.read(area_coord)) {
		setLastErrorString("Invalid map node.");
		return false;
	}

	uint16_t base_x = area_coord.x;
	uint16_t base_y = area_coord.y;
	uint16_t z = area_coord.z;

	for (auto& tileNode : tileAreaNode.children) {
		if (tileNode.type != OTBM_TILE && tileNode.type != OTBM_HOUSETILE) {
			setLastErrorString("Unknown tile node.");
			return false;
		}

		if (!loader.getProps(tileNode, propStream)) {
			setLastErrorString("Could not read node data.");
			return false;
		}

		OTBM_Tile_coords tile_coord;
		if (!propStream.read(tile_coord)) {
			setLastErrorString("Could not read tile position.");
			return false;
		}

		uint16_t x = base_x + tile_coord.x;
		uint16_t y = base_y + tile_coord.y;

		bool allowDecay = map.getTile(x, y, z) == nullptr || replaceExistingTiles;
		bool isHouseTile = false;
		House* house = nullptr;
		Tile* tile = nullptr;
		Item* ground_item = nullptr;
		uint32_t tileflags = TILESTATE_NONE;

		if (tileNode.type == OTBM_HOUSETILE) {
			uint32_t houseId;
			if (!propStream.read<uint32_t>(houseId)) {
				setLastErrorString(fmt::format("[x:{:d}, y:{:d}, z:{:d}] Could not read house id.", x, y, z));
				return false;
			}

			house = map.houses.addHouse(houseId);
			if (!house) {
				setLastErrorString(
				    fmt::format("[x:{:d}, y:{:d}, z:{:d}] Could not create house id: {:d}", x, y, z, houseId));
				return false;
			}

			tile = new Tile(x, y, z);
			tile->setHouse(house);
			house->addTile(tile);
			isHouseTile = true;
		}

		uint8_t attribute;
		// read tile attributes
		while (propStream.read<uint8_t>(attribute)) {
			switch (attribute) {
				case OTBM_ATTR_TILE_FLAGS: {
					uint32_t flags;
					if (!propStream.read<uint32_t>(flags)) {
						setLastErrorString(fmt::format("[x:{:d}, y:{:d}, z:{:d}] Failed to read tile flags.", x, y, z));
						return false;
					}

					if ((flags & OTBM_TILEFLAG_PROTECTIONZONE) != 0) {
						tileflags |= TILESTATE_PROTECTIONZONE;
					}

					// cannot be both
					if ((flags & OTBM_TILEFLAG_NOPVPZONE) != 0) {
						tileflags |= TILESTATE_NOPVPZONE;
					} else if ((flags & OTBM_TILEFLAG_PVPZONE) != 0) {
						tileflags |= TILESTATE_PVPZONE;
					}

					if ((flags & OTBM_TILEFLAG_REFRESH) != 0) {
						tileflags |= TILESTATE_REFRESH;
					}

					if ((flags & OTBM_TILEFLAG_NOLOGOUT) != 0) {
						tileflags |= TILESTATE_NOLOGOUT;
					}

					break;
				}

				case OTBM_ATTR_ITEM: {
					Item* item = Item::CreateItem(propStream);
					if (!item) {
						setLastErrorString(fmt::format("[x:{:d}, y:{:d}, z:{:d}] Failed to create item.", x, y, z));
						return false;
					}

					if (item->getItemCount() == 0) {
						item->setItemCount(1);
					}

					if (tile) {
						tile->internalAddThing(item);
						if (allowDecay) {
							item->startDecaying();
						}
					} else if (item->isGroundTile()) {
						delete ground_item;
						ground_item = item;
					} else {
						tile = createTile(ground_item, x, y, z);
						tile->internalAddThing(item);
						item->startDecaying();
					}
					break;
				}

				default:
					setLastErrorString(fmt::format("[x:{:d}, y:{:d}, z:{:d}] Unknown tile attribute.", x, y, z));
					return false;
			}
		}

		for (auto& itemNode : tileNode.children) {
			if (itemNode.type != OTBM_ITEM) {
				setLastErrorString(fmt::format("[x:{:d}, y:{:d}, z:{:d}] Unknown node type.", x, y, z));
				return false;
			}

			PropStream stream;
			if (!loader.getProps(itemNode, stream)) {
				setLastErrorString("Invalid item node.");
				return false;
			}

			Item* item = Item::CreateItem(stream);
			if (!item) {
				setLastErrorString(fmt::format("[x:{:d}, y:{:d}, z:{:d}] Failed to create item.", x, y, z));
				return false;
			}

			if (!item->unserializeItemNode(loader, itemNode, stream)) {
				setLastErrorString(
				    fmt::format("[x:{:d}, y:{:d}, z:{:d}] Failed to load item {:d}.", x, y, z, item->getID()));
				delete item;
				return false;
			}

			if (item->getItemCount() == 0) {
				item->setItemCount(1);
			}

			if (tile) {
				tile->internalAddThing(item);
				if (allowDecay) {
					item->startDecaying();
				}
			} else if (item->isGroundTile()) {
				delete ground_item;
				ground_item = item;
			} else {
				tile = createTile(ground_item, x, y, z);
				tile->internalAddThing(item);
				item->startDecaying();
			}
		}

		if (!tile) {
			tile = createTile(ground_item, x, y, z);
		}

		tile->setFlag(static_cast<tileflags_t>(tileflags));
		tile->makeRefreshItemList();

		map.setTile(x, y, z, tile, replaceExistingTiles);
	}
	return true;
}