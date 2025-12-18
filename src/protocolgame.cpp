// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include <boost/range/adaptor/reversed.hpp>

#include "protocolgame.h"

#include "outputmessage.h"

#include "player.h"

#include "configmanager.h"
#include "actions.h"
#include "game.h"
#include "iologindata.h"
#include "ban.h"
#include "scheduler.h"

#include <fmt/format.h>

extern ConfigManager g_config;
extern Actions actions;
extern CreatureEvents* g_creatureEvents;
extern Chat* g_chat;

namespace {

using WaitList = std::deque<std::pair<int64_t, uint32_t>>; // (timeout, player guid)

WaitList priorityWaitList, waitList;

std::tuple<WaitList&, WaitList::iterator, WaitList::size_type> findClient(const Player& player) {
	const auto fn = [&](const WaitList::value_type& it) { return it.second == player.getGUID(); };

	auto it = std::find_if(priorityWaitList.begin(), priorityWaitList.end(), fn);
	if (it != priorityWaitList.end()) {
		return std::make_tuple(std::ref(priorityWaitList), it, std::distance(it, priorityWaitList.end()) + 1);
	}

	it = std::find_if(waitList.begin(), waitList.end(), fn);
	if (it != waitList.end()) {
		return std::make_tuple(std::ref(waitList), it, priorityWaitList.size() + std::distance(it, waitList.end()) + 1);
	}

	return std::make_tuple(std::ref(waitList), waitList.end(), priorityWaitList.size() + waitList.size());
}

uint8_t getWaitTime(std::size_t slot)
{
	if (slot < 5) {
		return 5;
	} else if (slot < 10) {
		return 10;
	} else if (slot < 20) {
		return 20;
	} else if (slot < 50) {
		return 60;
	} else {
		return 120;
	}
}

int64_t getTimeout(std::size_t slot)
{
	// timeout is set to 15 seconds longer than expected retry attempt
	return getWaitTime(slot) + 15;
}

void cleanupList(WaitList& list)
{
	int64_t time = OTSYS_TIME();

	auto it = list.begin();
	while (it != list.end()) {
		if (it->first <= time) {
			it = list.erase(it);
		} else {
			++it;
		}
	}
}

std::size_t clientLogin(const Player& player)
{
	// Currentslot = position in wait list, 0 for direct access
	if (player.hasFlag(PlayerFlag_CanAlwaysLogin) || player.getAccountType() >= ACCOUNT_TYPE_GAMEMASTER) {
		return 0;
	}

	cleanupList(priorityWaitList);
	cleanupList(waitList);

	uint32_t maxPlayers = static_cast<uint32_t>(g_config.getNumber(ConfigManager::MAX_PLAYERS));
	if (maxPlayers == 0 || (priorityWaitList.empty() && waitList.empty() && g_game.getPlayersOnline() < maxPlayers)) {
		return 0;
	}

	auto result = findClient(player);
	if (std::get<1>(result) != std::get<0>(result).end()) {
		auto currentSlot = std::get<2>(result);
		// If server has capacity for this client, let him in even though his current slot might be higher than 0.
		if ((g_game.getPlayersOnline() + currentSlot) <= maxPlayers) {
			std::get<0>(result).erase(std::get<1>(result));
			return 0;
		}

		//let them wait a bit longer
		std::get<1>(result)->second = OTSYS_TIME() + (getTimeout(currentSlot) * 1000);
		return currentSlot;
	}

	auto currentSlot = priorityWaitList.size();
	if (player.isPremium()) {
		priorityWaitList.emplace_back(OTSYS_TIME() + (getTimeout(++currentSlot) * 1000), player.getGUID());
	} else {
		currentSlot += waitList.size();
		waitList.emplace_back(OTSYS_TIME() + (getTimeout(++currentSlot) * 1000), player.getGUID());
	}
	return currentSlot;
}

}

void ProtocolGame::release()
{
	//dispatcher thread
	if (player && player->client == shared_from_this()) {
		player->client.reset();
		player->decrementReferenceCounter();
		player = nullptr;
	}

	OutputMessagePool::getInstance().removeProtocolFromAutosend(shared_from_this());
	Protocol::release();
}

void ProtocolGame::login(const std::string& name, uint32_t accountId, OperatingSystem_t operatingSystem)
{
	// OTCv8 extended opcodes
	if (otclientV8 || operatingSystem >= CLIENTOS_OTCLIENT_LINUX) {
		NetworkMessage opcodeMessage;
		opcodeMessage.addByte(0x32);
		opcodeMessage.addByte(0x00);
		opcodeMessage.add<uint16_t>(0x00);
		writeToOutputBuffer(opcodeMessage);
	}

	//dispatcher thread
	Player* foundPlayer = g_game.getPlayerByName(name);
	if (!foundPlayer || g_config.getBoolean(ConfigManager::ALLOW_CLONES)) {
		player = new Player(getThis());
		player->setName(name);

		player->incrementReferenceCounter();
		player->setID();

		if (!IOLoginData::preloadPlayer(player, name)) {
			disconnectClient("Your character could not be loaded.");
			return;
		}

		if (IOBan::isPlayerNamelocked(player->getGUID())) {
			disconnectClient("Your character has been namelocked.");
			return;
		}

		if (g_game.getGameState() == GAME_STATE_CLOSING && !player->hasFlag(PlayerFlag_CanAlwaysLogin)) {
			disconnectClient("The game is just going down.\nPlease try again later.");
			return;
		}

		if (g_game.getGameState() == GAME_STATE_CLOSED && !player->hasFlag(PlayerFlag_CanAlwaysLogin)) {
			disconnectClient("Server is currently closed.\nPlease try again later.");
			return;
		}

		if (g_config.getBoolean(ConfigManager::ONE_PLAYER_ON_ACCOUNT) && player->getAccountType() < ACCOUNT_TYPE_GAMEMASTER && g_game.getPlayerByAccount(player->getAccount())) {
			disconnectClient("You may only login with one character\nof your account at the same time.");
			return;
		}

		if (!player->hasFlag(PlayerFlag_CannotBeBanned)) {
			BanInfo banInfo;
			if (IOBan::isAccountBanned(accountId, banInfo)) {
				if (banInfo.reason.empty()) {
					banInfo.reason = "(none)";
				}

				if (banInfo.expiresAt > 0) {
					disconnectClient(fmt::format("Your account has been banned until {:s} by {:s}.\n\nReason specified:\n{:s}", formatDateShort(banInfo.expiresAt), banInfo.bannedBy, banInfo.reason));
				} else {
					disconnectClient(fmt::format("Your account has been permanently banned by {:s}.\n\nReason specified:\n{:s}", banInfo.bannedBy, banInfo.reason));
				}
				return;
			}
		}

		if (std::size_t currentSlot = clientLogin(*player)) {
			uint8_t retryTime = getWaitTime(currentSlot);
			auto output = OutputMessagePool::getOutputMessage();
			output->addByte(0x16);
			output->addString(fmt::format("Too many players online.\nYou are at place {:d} on the waiting list.", currentSlot));
			output->addByte(retryTime);
			send(output);
			disconnect();
			return;
		}

		if (!IOLoginData::loadPlayer(player, true)) {
			disconnectClient("Your character could not be loaded.");
			return;
		}

		player->setOperatingSystem(operatingSystem);

		if (!g_game.placeCreature(player, player->getPosition())) {
			disconnectClient("Login failed due to corrupt data.");
			return;
		}

		if (operatingSystem >= CLIENTOS_OTCLIENT_LINUX) {
			player->registerCreatureEvent("ExtendedOpcode");
		}

		player->LastIPAddress = convertIPToString(getIP());

		if (g_config.getBoolean(ConfigManager::PLAYER_CONSOLE_LOGS)) {
			std::cout << player->getName() << " has logged in (IP:" << player->LastIPAddress << ")" << std::endl;
		}

		player->lastIP = player->getIP();
		player->lastLoginSaved = std::max<time_t>(time(nullptr), player->lastLoginSaved + 1);
		acceptPackets = true;
	} else {
		if (eventConnect != 0 || !g_config.getBoolean(ConfigManager::REPLACE_KICK_ON_LOGIN) || foundPlayer->isLoggingOut) {
			//Already trying to connect
			disconnectClient("You are already logged in.");
			return;
		}

		std::cout << foundPlayer->getName() << " has a new client. (IP:" << convertIPToString(getIP()) << ')' << std::endl;

		foundPlayer->incrementReferenceCounter();
		if (foundPlayer->client) {
			foundPlayer->disconnect();
			foundPlayer->resetIdleTime();
			foundPlayer->lastPing = OTSYS_TIME();
			foundPlayer->isConnecting = true;

			eventConnect = g_scheduler.addEvent(createSchedulerTask(1000, std::bind(&ProtocolGame::connect, getThis(), foundPlayer->getID(), operatingSystem)));
		} else {
			connect(foundPlayer->getID(), operatingSystem);
		}
	}
	OutputMessagePool::getInstance().addProtocolToAutosend(shared_from_this());
}

void ProtocolGame::connect(uint32_t playerId, OperatingSystem_t operatingSystem)
{
	eventConnect = 0;

	Player* foundPlayer = g_game.getPlayerByID(playerId);
	if (!foundPlayer || foundPlayer->client || foundPlayer->isLoggingOut) {
		disconnectClient("You are already logged in.");
		return;
	}

	foundPlayer->decrementReferenceCounter();

	if (isConnectionExpired()) {
		//ProtocolGame::release() has been called at this point and the Connection object
		//no longer exists, so we return to prevent leakage of the Player.
		return;
	}

	player = foundPlayer;
	player->incrementReferenceCounter();

	g_chat->removeUserFromAllChannels(*player);
	player->clearModalWindows();
	player->setOperatingSystem(operatingSystem);
	player->isConnecting = false;

	player->lastPing = OTSYS_TIME();
	player->resetIdleTime();
	player->client = getThis();
	sendAddCreature(player, player->getPosition(), 0);
	player->lastIP = player->getIP();
	player->lastLoginSaved = std::max<time_t>(time(nullptr), player->lastLoginSaved + 1);
	acceptPackets = true;
}

void ProtocolGame::logout(bool forced)
{
	//dispatcher thread
	if (!player || player->isLoggingOut) {
		return;
	}

	if (!player->isRemoved()) {
		if (!forced) {
			if (!player->isAccessPlayer()) {
				if (player->getTile()->hasFlag(TILESTATE_NOLOGOUT)) {
					player->sendCancelMessage(RETURNVALUE_YOUCANNOTLOGOUTHERE);
					return;
				}

				if (player->hasCondition(CONDITION_INFIGHT)) {
					player->sendCancelMessage(RETURNVALUE_YOUMAYNOTLOGOUTDURINGAFIGHT);
					return;
				}
			}

			//scripting event - onLogout
			if (!g_creatureEvents->playerLogout(player)) {
				//Let the script handle the error message
				return;
			}
		}

		player->isLoggingOut = true;
		g_game.executeRemoveCreature(player);
	} else {
		disconnect();
	}
}

void ProtocolGame::onRecvFirstMessage(NetworkMessage& msg)
{
	if (g_game.getGameState() == GAME_STATE_SHUTDOWN) {
		disconnect();
		return;
	}

	OperatingSystem_t operatingSystem = static_cast<OperatingSystem_t>(msg.get<uint16_t>());
	version = msg.get<uint16_t>();

	if (!Protocol::RSA_decrypt(msg)) {
		disconnect();
		return;
	}

	xtea::key key;
	key[0] = msg.get<uint32_t>();
	key[1] = msg.get<uint32_t>();
	key[2] = msg.get<uint32_t>();
	key[3] = msg.get<uint32_t>();
	enableXTEAEncryption();
	setXTEAKey(std::move(key));

	if (g_game.isIPLocked(getIP())) {
		disconnectClient(g_config.getString(ConfigManager::IP_LOCK_MESSAGE));
		return;
	}

	if (operatingSystem >= CLIENTOS_OTCLIENT_LINUX) {
		NetworkMessage opcodeMessage;
		opcodeMessage.addByte(0x32);
		opcodeMessage.addByte(0x00);
		opcodeMessage.add<uint16_t>(0x00);
		writeToOutputBuffer(opcodeMessage);
	}

	msg.skipBytes(1); // gamemaster flag
	uint32_t accountNumber = msg.get<uint32_t>();
	std::string character = msg.getString();
	std::string password = msg.getString();

	uint16_t otcv8StringLength = msg.get<uint16_t>();
	if (otcv8StringLength == 5 && msg.getString(5) == "OTCv8") {
		otclientV8 = msg.get<uint16_t>();
	}

	if (accountNumber == 0) {
		disconnectClient("You must enter your account number.");
		return;
	}

	if (version < CLIENT_VERSION_MIN || version > CLIENT_VERSION_MAX) {
		disconnectClient(fmt::format("Only clients with protocol {:s} allowed!", CLIENT_VERSION_STR));
		return;
	}

	if (g_game.getGameState() == GAME_STATE_STARTUP) {
		disconnectClient("Gameworld is starting up. Please wait.");
		return;
	}

	if (g_game.getGameState() == GAME_STATE_MAINTAIN) {
		disconnectClient("Gameworld is under maintenance. Please re-connect in a while.");
		return;
	}

	if (g_game.isAccountLocked(accountNumber)) {
		disconnectClient(g_config.getString(ConfigManager::ACCOUNT_LOCK_MESSAGE));
		return;
	}

	BanInfo banInfo;
	if (IOBan::isIpBanned(getIP(), banInfo)) {
		if (banInfo.reason.empty()) {
			banInfo.reason = "(none)";
		}

		disconnectClient(fmt::format("Your IP has been banned until {:s} by {:s}.\n\nReason specified:\n{:s}", formatDateShort(banInfo.expiresAt), banInfo.bannedBy, banInfo.reason));
		return;
	}

	uint32_t accountId = IOLoginData::gameworldAuthentication(accountNumber, password, character);
	if (accountId == 0) {
		g_game.registerFailedIPLogin(getIP());
		g_game.registerFailedAccountLogin(accountNumber);

		disconnectClient("Account number or password is not correct.");
		return;
	}

	g_game.resetAccountLoginAttempts(accountNumber);
	g_game.resetIpLoginAttempts(getIP());

	g_dispatcher.addTask(createTask(std::bind(&ProtocolGame::login, getThis(), character, accountId, operatingSystem)));
}

void ProtocolGame::disconnectClient(const std::string& message) const
{
	auto output = OutputMessagePool::getOutputMessage();
	output->addByte(0x14);
	output->addString(message);
	send(output);
	disconnect();
}

void ProtocolGame::disconnect() const
{
	if (player && g_config.getBoolean(ConfigManager::PLAYER_CONSOLE_LOGS)) {
		std::cout << player->getName() << "' client disconnected (IP:" << convertIPToString(getIP()) << ")" << std::endl;
	}

	Protocol::disconnect();
}

void ProtocolGame::writeToOutputBuffer(const NetworkMessage& msg)
{
	auto out = getOutputBuffer(msg.getLength());
	out->append(msg);
}

void ProtocolGame::parsePacket(NetworkMessage& msg)
{
	g_dispatcher.addTask(createTask(std::bind(&ProtocolGame::parsePacketOnDispatcher, this, std::move(msg))));
}

void ProtocolGame::parsePacketOnDispatcher(NetworkMessage msg)
{
	if (!acceptPackets || g_game.getGameState() == GAME_STATE_SHUTDOWN || msg.getLength() == 0) {
		return;
	}

	uint8_t recvbyte = msg.getByte();

	if (!player) {
		if (recvbyte == 0x0F) {
			disconnect();
		}

		return;
	}

	//a dead player can not performs actions
	if (player->isRemoved() || player->getHealth() <= 0) {
		if (recvbyte == 0x0F) {
			disconnect();
			return;
		}

		if (recvbyte != 0x14) {
			return;
		}
	}

	switch (recvbyte) {
	case 0x14: logout(false); break;
	case 0x1D: g_game.playerReceivePingBack(player->getID()); break;
	case 0x1E: g_game.playerReceivePing(player->getID()); break;
	case 0x32: parseExtendedOpcode(msg); break; //otclient extended opcode
	case 0x64: parseAutoWalk(msg); break;
	case 0x65: g_game.playerMove(player->getID(), DIRECTION_NORTH); break;
	case 0x66: g_game.playerMove(player->getID(), DIRECTION_EAST); break;
	case 0x67: g_game.playerMove(player->getID(), DIRECTION_SOUTH); break;
	case 0x68: g_game.playerMove(player->getID(), DIRECTION_WEST); break;
	case 0x69: g_game.playerStopAutoWalk(player->getID()); break;
	case 0x6A: g_game.playerMove(player->getID(), DIRECTION_NORTHEAST); break;
	case 0x6B: g_game.playerMove(player->getID(), DIRECTION_SOUTHEAST); break;
	case 0x6C: g_game.playerMove(player->getID(), DIRECTION_SOUTHWEST); break;
	case 0x6D: g_game.playerMove(player->getID(), DIRECTION_NORTHWEST); break;
	case 0x6F: g_game.playerTurn(player->getID(), DIRECTION_NORTH); break;
	case 0x70: g_game.playerTurn(player->getID(), DIRECTION_EAST); break;
	case 0x71: g_game.playerTurn(player->getID(), DIRECTION_SOUTH); break;
	case 0x72: g_game.playerTurn(player->getID(), DIRECTION_WEST); break;
	case 0x78: parseThrow(msg); break;
	case 0x7D: parseRequestTrade(msg); break;
	case 0x7E: parseLookInTrade(msg); break;
	case 0x7F: g_game.playerAcceptTrade(player->getID()); break;
	case 0x80: g_game.playerCloseTrade(player->getID()); break;
	case 0x82: parseUseItem(msg); break;
	case 0x83: parseUseItemEx(msg); break;
	case 0x84: parseUseWithCreature(msg); break;
	case 0x85: parseRotateItem(msg); break;
	case 0x87: parseCloseContainer(msg); break;
	case 0x88: parseUpArrowContainer(msg); break;
	case 0x89: parseTextWindow(msg); break;
	case 0x8A: parseHouseWindow(msg); break;
	case 0x8C: parseLookAt(msg); break;
	case 0x8D: parseLookInBattleList(msg); break;
	case 0x96: parseSay(msg); break;
	case 0x97: g_game.playerRequestChannels(player->getID()); break;
	case 0x98: parseOpenChannel(msg); break;
	case 0x99: parseCloseChannel(msg); break;
	case 0x9A: parseOpenPrivateChannel(msg); break;
	case 0x9B: parseProcessRuleViolationReport(msg); break;
	case 0x9C: parseCloseRuleViolationReport(msg); break;
	case 0x9D: addGameTask(&Game::playerCancelRuleViolationReport, player->getID()); break;
	case 0xA0: parseFightModes(msg); break;
	case 0xA1: parseAttack(msg); break;
	case 0xA2: parseFollow(msg); break;
	case 0xA3: parseInviteToParty(msg); break;
	case 0xA4: parseJoinParty(msg); break;
	case 0xA5: parseRevokePartyInvite(msg); break;
	case 0xA6: parsePassPartyLeadership(msg); break;
	case 0xA7: g_game.playerLeaveParty(player->getID()); break;
	case 0xA8: parseEnableSharedPartyExperience(msg); break;
	case 0xAA: g_game.playerCreatePrivateChannel(player->getID()); break;
	case 0xAB: parseChannelInvite(msg); break;
	case 0xAC: parseChannelExclude(msg); break;
	case 0xBE: g_game.playerCancelAttackAndFollow(player->getID()); break;
	case 0xC9: /* update tile */ break;
	case 0xCA: parseUpdateContainer(msg); break;
	case 0xD2: g_game.playerRequestOutfit(player->getID()); break;
	case 0xD3: parseSetOutfit(msg); break;
	case 0xDC: parseAddVip(msg); break;
	case 0xDD: parseRemoveVip(msg); break;
	case 0xE6: parseBugReport(msg); break;
	case 0xE8: parseDebugAssert(msg); break;
	case 0xF9: parseModalWindowAnswer(msg); break;

	default:
		std::cout << "Player: " << player->getName() << " sent an unknown packet header: 0x" << std::hex << static_cast<uint16_t>(recvbyte) << std::dec << "!" << std::endl;
		break;
	}

	if (msg.isOverrun()) {
		disconnect();
	}
}

void ProtocolGame::GetTileDescription(const Tile* tile, NetworkMessage& msg)
{
	int32_t count;
	Item* ground = tile->getGround();
	if (ground) {
		msg.addItem(ground);
		count = 1;
	} else {
		count = 0;
	}

	const TileItemVector* items = tile->getItemList();
	if (items) {
		for (auto it = items->getBeginTopItem(), end = items->getEndTopItem(); it != end; ++it) {
			msg.addItem(*it);

			if (++count == 10) {
				break;
			}
		}
	}

	const CreatureVector* creatures = tile->getCreatures();
	if (creatures) {
		for (const Creature* creature : boost::adaptors::reverse(*creatures)) {
			if (!player->canSeeCreature(creature)) {
				continue;
			}

			bool known;
			uint32_t removedKnown;
			checkCreatureAsKnown(creature->getID(), known, removedKnown);
			AddCreature(msg, creature, known, removedKnown);
			if (++count == 10) {
				return;
			}
		}
	}

	if (items && count < 10) {
		for (auto it = items->getBeginDownItem(), end = items->getEndDownItem(); it != end; ++it) {
			msg.addItem(*it);

			if (++count == 10) {
				return;
			}
		}
	}
}

void ProtocolGame::GetMapDescription(int32_t x, int32_t y, int32_t z, int32_t width, int32_t height, NetworkMessage& msg)
{
	int32_t skip = -1;
	int32_t startz, endz, zstep;

	if (z > 7) {
		startz = z - 2;
		endz = std::min<int32_t>(MAP_MAX_LAYERS - 1, z + 2);
		zstep = 1;
	} else {
		startz = 7;
		endz = 0;
		zstep = -1;
	}

	for (int32_t nz = startz; nz != endz + zstep; nz += zstep) {
		GetFloorDescription(msg, x, y, nz, width, height, z - nz, skip);
	}

	if (skip >= 0) {
		msg.addByte(skip);
		msg.addByte(0xFF);
	}
}

void ProtocolGame::GetFloorDescription(NetworkMessage& msg, int32_t x, int32_t y, int32_t z, int32_t width, int32_t height, int32_t offset, int32_t& skip)
{
	for (int32_t nx = 0; nx < width; nx++) {
		for (int32_t ny = 0; ny < height; ny++) {
			Tile* tile = g_game.map.getTile(x + nx + offset, y + ny + offset, z);
			if (tile) {
				if (skip >= 0) {
					msg.addByte(skip);
					msg.addByte(0xFF);
				}

				skip = 0;
				GetTileDescription(tile, msg);
			} else if (skip == 0xFE) {
				msg.addByte(0xFF);
				msg.addByte(0xFF);
				skip = -1;
			} else {
				++skip;
			}
		}
	}
}

void ProtocolGame::checkCreatureAsKnown(uint32_t id, bool& known, uint32_t& removedKnown)
{
	auto result = knownCreatureSet.insert(id);
	if (!result.second) {
		known = true;
		return;
	}

	known = false;

	if (knownCreatureSet.size() > 150) {
		// Look for a creature to remove
		for (auto it = knownCreatureSet.begin(), end = knownCreatureSet.end(); it != end; ++it) {
			Creature* creature = g_game.getCreatureByID(*it);
			if (!canSee(creature)) {
				removedKnown = *it;
				knownCreatureSet.erase(it);
				return;
			}
		}

		// Bad situation. Let's just remove anyone.
		auto it = knownCreatureSet.begin();
		if (*it == id) {
			++it;
		}

		removedKnown = *it;
		knownCreatureSet.erase(it);
	} else {
		removedKnown = 0;
	}
}

bool ProtocolGame::canSee(const Creature* c) const
{
	if (!c || !player || c->isRemoved()) {
		return false;
	}

	if (!player->canSeeCreature(c)) {
		return false;
	}

	return canSee(c->getPosition());
}

bool ProtocolGame::canSee(const Position& pos) const
{
	return canSee(pos.x, pos.y, pos.z);
}

bool ProtocolGame::isVisible(int32_t x, int32_t y, int32_t z) const
{
	if (!player) {
		return false;
	}

	const Position& myPos = player->getPosition();
	if (myPos.z <= 7) {
		//we are on ground level or above (7 -> 0)
		//view is from 7 -> 0
		if (z > 7) {
			return false;
		}
	} else { // if (myPos.z >= 8) {
		//we are underground (8 -> 15)
		//view is +/- 2 from the floor we stand on
		if (std::abs(myPos.getZ() - z) > 2) {
			return false;
		}
	}

	int32_t dy = Map::maxClientViewportY + y - myPos.getY() - (myPos.getZ() - z);
	int32_t dx = Map::maxClientViewportX + x - myPos.getX() - (myPos.getZ() - z);
	return dx >= 0 && dx < CLIENT_TERMINAL_WIDTH && dy >= 0 && dy < CLIENT_TERMINAL_HEIGHT;
}

bool ProtocolGame::canSee(int32_t x, int32_t y, int32_t z) const
{
	if (!player) {
		return false;
	}

	const Position& myPos = player->getPosition();
	if (myPos.z <= 7) {
		//we are on ground level or above (7 -> 0)
		//view is from 7 -> 0
		if (z > 7) {
			return false;
		}
	} else { // if (myPos.z >= 8) {
		//we are underground (8 -> 15)
		//view is +/- 2 from the floor we stand on
		if (std::abs(myPos.getZ() - z) > 2) {
			return false;
		}
	}

	//negative offset means that the action taken place is on a lower floor than ourself
	int32_t offsetz = myPos.getZ() - z;
	if ((x >= myPos.getX() - Map::maxClientViewportX + offsetz) && (x <= myPos.getX() + (Map::maxClientViewportX + 1) + offsetz) &&
		(y >= myPos.getY() - Map::maxClientViewportY + offsetz) && (y <= myPos.getY() + (Map::maxClientViewportY + 1) + offsetz)) {
		return true;
	}
	return false;
}

// Parse methods
void ProtocolGame::parseChannelInvite(NetworkMessage& msg)
{
	const std::string name = msg.getString();
	g_game.playerChannelInvite(player->getID(), name);
}

void ProtocolGame::parseChannelExclude(NetworkMessage& msg)
{
	const std::string name = msg.getString();
	g_game.playerChannelExclude(player->getID(), name);
}

void ProtocolGame::parseOpenChannel(NetworkMessage& msg)
{
	uint16_t channelId = msg.get<uint16_t>();
	g_game.playerOpenChannel(player->getID(), channelId);
}

void ProtocolGame::parseCloseChannel(NetworkMessage& msg)
{
	uint16_t channelId = msg.get<uint16_t>();
	g_game.playerCloseChannel(player->getID(), channelId);
}

void ProtocolGame::parseOpenPrivateChannel(NetworkMessage& msg)
{
	std::string receiver = msg.getString();
	g_game.playerOpenPrivateChannel(player->getID(), receiver);
}

void ProtocolGame::parseAutoWalk(NetworkMessage& msg)
{
	uint8_t numdirs = msg.getByte();
	if (numdirs == 0 || (msg.getBufferPosition() + numdirs) != (msg.getLength() + 4) || numdirs > 128) {
		return;
	}

	msg.skipBytes(numdirs);

	std::vector<Direction> path;
	path.reserve(numdirs);

	for (uint8_t i = 0; i < numdirs; ++i) {
		uint8_t rawdir = msg.getPreviousByte();
		switch (rawdir) {
			case 1: path.push_back(DIRECTION_EAST); break;
			case 2: path.push_back(DIRECTION_NORTHEAST); break;
			case 3: path.push_back(DIRECTION_NORTH); break;
			case 4: path.push_back(DIRECTION_NORTHWEST); break;
			case 5: path.push_back(DIRECTION_WEST); break;
			case 6: path.push_back(DIRECTION_SOUTHWEST); break;
			case 7: path.push_back(DIRECTION_SOUTH); break;
			case 8: path.push_back(DIRECTION_SOUTHEAST); break;
			default: break;
		}
	}

	if (path.empty()) {
		return;
	}

	std::reverse(path.begin(), path.end());
	g_game.playerAutoWalk(player->getID(), path);
}

void ProtocolGame::parseSetOutfit(NetworkMessage& msg)
{
	Outfit_t newOutfit;
	newOutfit.lookType = msg.get<uint16_t>();
	newOutfit.lookHead = msg.getByte();
	newOutfit.lookBody = msg.getByte();
	newOutfit.lookLegs = msg.getByte();
	newOutfit.lookFeet = msg.getByte();
	g_game.playerChangeOutfit(player->getID(), newOutfit);
}

void ProtocolGame::parseUseItem(NetworkMessage& msg)
{
	Position pos = msg.getPosition();
	uint16_t spriteId = msg.get<uint16_t>();
	uint8_t stackpos = msg.getByte();
	uint8_t index = msg.getByte();

	// GameBeat Related
	if (g_config.getBoolean(ConfigManager::GAME_BEAT_SIMULATION)) {
		if (player->isExecuting) {
			if (uniform_random(0, 100) <= 10) {
				g_game.playerUseItem(player->getID(), pos, stackpos, index, spriteId);

				sendCancelWalk();

				player->clearToDo();
				player->addWaitToDo(100);
				player->startToDo();
				return;
			}

			sendCancelWalk();
		}
	}

	player->addWaitToDo(g_config.getNumber(ConfigManager::ACTIONS_DELAY_INTERVAL));
	player->addActionToDo(std::bind(&Game::playerUseItem, &g_game, player->getID(), pos, stackpos, index, spriteId));
	player->startToDo();
}

void ProtocolGame::parseUseItemEx(NetworkMessage& msg)
{
	Position fromPos = msg.getPosition();
	uint16_t fromSpriteId = msg.get<uint16_t>();
	uint8_t fromStackPos = msg.getByte();
	Position toPos = msg.getPosition();
	uint16_t toSpriteId = msg.get<uint16_t>();
	uint8_t toStackPos = msg.getByte();

	// GameBeat Related
	if (g_config.getBoolean(ConfigManager::GAME_BEAT_SIMULATION)) {
		if (player->isExecuting) {
			if (uniform_random(0, 100) <= 10) {
				g_game.playerUseItemEx(player->getID(), fromPos, fromStackPos, fromSpriteId, toPos, toStackPos, toSpriteId);
				
				sendCancelWalk();

				player->clearToDo();
				player->addWaitToDo(100);
				player->startToDo();
				return;
			}
		}
	}

	player->addWaitToDo(g_config.getNumber(ConfigManager::ACTIONS_DELAY_INTERVAL));
	player->addActionToDo(TODO_USEEX, std::bind(&Game::playerUseItemEx, &g_game, player->getID(), fromPos, fromStackPos, fromSpriteId, toPos, toStackPos, toSpriteId));
	player->startToDo();
}

void ProtocolGame::parseUseWithCreature(NetworkMessage& msg)
{
	Position fromPos = msg.getPosition();
	uint16_t spriteId = msg.get<uint16_t>();
	uint8_t fromStackPos = msg.getByte();
	uint32_t creatureId = msg.get<uint32_t>();

	// GameBeat Related
	if (g_config.getBoolean(ConfigManager::GAME_BEAT_SIMULATION)) {
		if (player->isExecuting) {
			if (uniform_random(0, 100) <= 10) {
				g_game.playerUseWithCreature(player->getID(), fromPos, fromStackPos, creatureId, spriteId);
				
				sendCancelWalk();

				player->clearToDo();
				player->addWaitToDo(100);
				player->startToDo();
				return;
			}
		}
	}

	player->addWaitToDo(g_config.getNumber(ConfigManager::ACTIONS_DELAY_INTERVAL));
	player->addActionToDo(TODO_USEEX, std::bind(&Game::playerUseWithCreature, &g_game, player->getID(), fromPos, fromStackPos, creatureId, spriteId));
	player->startToDo();
}

void ProtocolGame::parseCloseContainer(NetworkMessage& msg)
{
	uint8_t cid = msg.getByte();
	g_game.playerCloseContainer(player->getID(), cid);
}

void ProtocolGame::parseUpArrowContainer(NetworkMessage& msg)
{
	uint8_t cid = msg.getByte();
	g_game.playerMoveUpContainer(player->getID(), cid);
}

void ProtocolGame::parseUpdateContainer(NetworkMessage& msg)
{
	uint8_t cid = msg.getByte();
	g_game.playerUpdateContainer(player->getID(), cid);
}

void ProtocolGame::parseThrow(NetworkMessage& msg)
{
	Position fromPos = msg.getPosition();
	uint16_t spriteId = msg.get<uint16_t>();
	uint8_t fromStackpos = msg.getByte();
	Position toPos = msg.getPosition();
	uint8_t count = msg.getByte();

	if (toPos != fromPos) {
		if (g_config.getBoolean(ConfigManager::GAME_BEAT_SIMULATION)) {
			// Game Beat behavior simulation
			if (player->isExecuting) {
				if (uniform_random(0, 100) <= 10) {
					g_game.playerMoveThing(player->getID(), fromPos, spriteId, fromStackpos, toPos, count);
					
					sendCancelWalk();

					player->clearToDo();
					player->addWaitToDo(100);
					player->startToDo();
					return;
				}
			}
		}

		if (spriteId != 99) {
			player->addWaitToDo(100);
		}

		player->addActionToDo(std::bind(&Game::playerMoveThing, &g_game, player->getID(), fromPos, spriteId, fromStackpos, toPos, count));
		player->startToDo();
	}
}

void ProtocolGame::parseLookAt(NetworkMessage& msg)
{
	Position pos = msg.getPosition();
	msg.skipBytes(2); // spriteId
	uint8_t stackpos = msg.getByte();

	// It is queued, GameBeat related.
	g_scheduler.addEvent(createSchedulerTask(50, std::bind(&Game::playerLookAt, &g_game, player->getID(), pos, stackpos)));
}

void ProtocolGame::parseLookInBattleList(NetworkMessage& msg)
{
	uint32_t creatureId = msg.get<uint32_t>();
	g_game.playerLookInBattleList(player->getID(), creatureId);
}

void ProtocolGame::parseSay(NetworkMessage& msg)
{
	std::string receiver;
	uint16_t channelId;

	SpeakClasses type = static_cast<SpeakClasses>(msg.getByte());
	switch (type) {
		case TALKTYPE_PRIVATE:
		case TALKTYPE_PRIVATE_RED:
		case TALKTYPE_RVR_ANSWER:
			receiver = msg.getString();
			channelId = 0;
			break;

		case TALKTYPE_CHANNEL_Y:
		case TALKTYPE_CHANNEL_R1:
		case TALKTYPE_CHANNEL_R2:
			channelId = msg.get<uint16_t>();
			break;

		default:
			channelId = 0;
			break;
	}

	const std::string text = msg.getString();
	if (text.length() > 255) {
		return;
	}

	// OTC does not automatically schedule auto walking upon talking, only real Tibia client does
	if (otclientV8) {
		g_game.playerSay(player->getID(), channelId, type, receiver, text);
	} else {
		if (player->isExecuting && player->clearToDo()) {
			sendCancelWalk();
		}

		player->addActionToDo(std::bind(&Game::playerSay, &g_game, player->getID(), channelId, type, receiver, text));
		player->startToDo();
	}
}

void ProtocolGame::parseFightModes(NetworkMessage& msg)
{
	uint8_t rawFightMode = msg.getByte(); // 1 - offensive, 2 - balanced, 3 - defensive
	uint8_t rawChaseMode = msg.getByte(); // 0 - stand while fighting, 1 - chase opponent
	uint8_t rawSecureMode = msg.getByte(); // 0 - can't attack unmarked, 1 - can attack unmarked

	fightMode_t fightMode;
	if (rawFightMode == 1) {
		fightMode = FIGHTMODE_ATTACK;
	} else if (rawFightMode == 2) {
		fightMode = FIGHTMODE_BALANCED;
	} else {
		fightMode = FIGHTMODE_DEFENSE;
	}

	g_game.playerSetFightModes(player->getID(), fightMode, rawChaseMode != 0, rawSecureMode != 0);
}

void ProtocolGame::parseAttack(NetworkMessage& msg)
{
	if (player->isExecuting && player->clearToDo()) {
		sendCancelWalk();
	}
	
	uint32_t creatureId = msg.get<uint32_t>();
	g_game.playerSetAttackedCreature(player->getID(), creatureId);
}

void ProtocolGame::parseFollow(NetworkMessage& msg)
{
	uint32_t creatureId = msg.get<uint32_t>();
	g_game.playerFollowCreature(player->getID(), creatureId);
}

void ProtocolGame::parseProcessRuleViolationReport(NetworkMessage& msg)
{
	const std::string reporter = msg.getString();
	addGameTask(&Game::playerProcessRuleViolationReport, player->getID(), reporter);
}

void ProtocolGame::parseCloseRuleViolationReport(NetworkMessage& msg)
{
	const std::string reporter = msg.getString();
	addGameTask(&Game::playerCloseRuleViolationReport, player->getID(), reporter);
}

void ProtocolGame::parseTextWindow(NetworkMessage& msg)
{
	uint32_t windowTextId = msg.get<uint32_t>();
	const std::string newText = msg.getString();
	g_game.playerWriteItem(player->getID(), windowTextId, newText);
}

void ProtocolGame::parseHouseWindow(NetworkMessage& msg)
{
	uint8_t doorId = msg.getByte();
	uint32_t id = msg.get<uint32_t>();
	const std::string text = msg.getString();
	g_game.playerUpdateHouseWindow(player->getID(), doorId, id, text);
}

void ProtocolGame::parseRequestTrade(NetworkMessage& msg)
{
	Position pos = msg.getPosition();
	uint16_t spriteId = msg.get<uint16_t>();
	uint8_t stackpos = msg.getByte();
	uint32_t playerId = msg.get<uint32_t>();

	player->addActionToDo(std::bind(&Game::playerRequestTrade, &g_game, player->getID(), pos, stackpos, playerId, spriteId));
	player->startToDo();
}

void ProtocolGame::parseLookInTrade(NetworkMessage& msg)
{
	bool counterOffer = (msg.getByte() == 0x01);
	uint8_t index = msg.getByte();
	g_game.playerLookInTrade(player->getID(), counterOffer, index);
}

void ProtocolGame::parseAddVip(NetworkMessage& msg)
{
	const std::string name = msg.getString();
	g_game.playerRequestAddVip(player->getID(), name);
}

void ProtocolGame::parseRemoveVip(NetworkMessage& msg)
{
	uint32_t guid = msg.get<uint32_t>();
	g_game.playerRequestRemoveVip(player->getID(), guid);
}

void ProtocolGame::parseRotateItem(NetworkMessage& msg)
{
	Position pos = msg.getPosition();
	uint16_t spriteId = msg.get<uint16_t>();
	uint8_t stackpos = msg.getByte();

	player->addActionToDo(std::bind(&Game::playerRotateItem, &g_game, player->getID(), pos, stackpos, spriteId));
	player->startToDo();
}

void ProtocolGame::parseBugReport(NetworkMessage& msg)
{
	std::string message = msg.getString();
	g_game.playerReportBug(player->getID(), message);
}

void ProtocolGame::parseDebugAssert(NetworkMessage& msg)
{
	if (debugAssertSent) {
		return;
	}

	debugAssertSent = true;

	std::string assertLine = msg.getString();
	std::string date = msg.getString();
	std::string description = msg.getString();
	std::string comment = msg.getString();
	g_game.playerDebugAssert(player->getID(), assertLine, date, description, comment);
}

void ProtocolGame::parseInviteToParty(NetworkMessage& msg)
{
	uint32_t targetId = msg.get<uint32_t>();
	g_game.playerInviteToParty(player->getID(), targetId);
}

void ProtocolGame::parseJoinParty(NetworkMessage& msg)
{
	uint32_t targetId = msg.get<uint32_t>();
	g_game.playerJoinParty(player->getID(), targetId);
}

void ProtocolGame::parseRevokePartyInvite(NetworkMessage& msg)
{
	uint32_t targetId = msg.get<uint32_t>();
	g_game.playerRevokePartyInvitation(player->getID(), targetId);
}

void ProtocolGame::parsePassPartyLeadership(NetworkMessage& msg)
{
	uint32_t targetId = msg.get<uint32_t>();
	g_game.playerPassPartyLeadership(player->getID(), targetId);
}

void ProtocolGame::parseModalWindowAnswer(NetworkMessage& msg)
{
	uint32_t id = msg.get<uint32_t>();
	uint8_t button = msg.getByte();
	uint8_t choice = msg.getByte();
	addGameTask(&Game::playerAnswerModalWindow, player->getID(), id, button, choice);
}

void ProtocolGame::parseEnableSharedPartyExperience(NetworkMessage& msg)
{
	bool sharedExpActive = msg.getByte() == 1;
	g_game.playerEnableSharedPartyExperience(player->getID(), sharedExpActive);
}

// Send methods
void ProtocolGame::sendOpenPrivateChannel(const std::string& receiver)
{
	NetworkMessage msg;
	msg.addByte(0xAD);
	msg.addString(receiver);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureOutfit(const Creature* creature, const Outfit_t& outfit)
{
	if (!canSee(creature)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x8E);
	msg.add<uint32_t>(creature->getID());
	AddOutfit(msg, outfit);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureLight(const Creature* creature)
{
	if (!canSee(creature)) {
		return;
	}

	NetworkMessage msg;
	AddCreatureLight(msg, creature);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendWorldLight(LightInfo lightInfo)
{
	NetworkMessage msg;
	AddWorldLight(msg, lightInfo);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureShield(const Creature* creature)
{
	if (!canSee(creature)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x91);
	msg.add<uint32_t>(creature->getID());
	msg.addByte(player->getPartyShield(creature->getPlayer()));
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureSkull(const Creature* creature)
{
	if (g_game.getWorldType() != WORLD_TYPE_PVP) {
		return;
	}

	if (!canSee(creature)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x90);
	msg.add<uint32_t>(creature->getID());
	msg.addByte(player->getSkullClient(creature));
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureSquare(const Creature* creature, SquareColor_t color)
{
	if (!canSee(creature)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x86);
	msg.add<uint32_t>(creature->getID());
	msg.addByte(color);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendRemoveRuleViolationReport(const std::string& name)
{
	NetworkMessage msg;
	msg.addByte(0xAF);
	msg.addString(name);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendLockRuleViolation()
{
	NetworkMessage msg;
	msg.addByte(0xB1);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendRuleViolationCancel(const std::string& name)
{
	NetworkMessage msg;
	msg.addByte(0xB0);
	msg.addString(name);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendRuleViolationsChannel(uint16_t channelId)
{
	NetworkMessage msg;
	msg.addByte(0xAE);
	msg.add<uint16_t>(channelId);

	std::vector<RuleViolation> reports;

	auto it = g_game.getRuleViolationReports().begin();
	for (const auto& it : g_game.getRuleViolationReports()) {
		reports.push_back(it.second);
	}

	// order reports
	std::sort(reports.begin(), reports.end(), [](const RuleViolation& a, const RuleViolation& b) {
		return a.timestamp < b.timestamp;
		});

	for (const RuleViolation& rvr : reports) {
		Player* reporter = g_game.getPlayerByID(rvr.reporterId);
		if (reporter) {
			msg.addByte(0xAA);
			msg.add<uint32_t>(0);
			msg.addString(reporter->getName());
			msg.addByte(TALKTYPE_RVR_CHANNEL);
			msg.add<uint32_t>(rvr.timestamp);
			msg.addString(rvr.text);
		}
	}

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendStats()
{
	NetworkMessage msg;
	AddPlayerStats(msg);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendTextMessage(const TextMessage& message)
{
	NetworkMessage msg;
	msg.addByte(0xB4);
	msg.addByte(message.type);
	msg.addString(message.text);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendAnimatedText(const Position& pos, uint8_t color, const std::string& text)
{
	if (!isVisible(pos.x, pos.y, pos.z)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x84);
	msg.addPosition(pos);
	msg.addByte(color);
	msg.addString(text);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendClosePrivate(uint16_t channelId)
{
	NetworkMessage msg;
	msg.addByte(0xB3);
	msg.add<uint16_t>(channelId);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatePrivateChannel(uint16_t channelId, const std::string& channelName)
{
	NetworkMessage msg;
	msg.addByte(0xB2);
	msg.add<uint16_t>(channelId);
	msg.addString(channelName);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendChannelsDialog()
{
	NetworkMessage msg;
	msg.addByte(0xAB);

	const ChannelList& list = g_chat->getChannelList(*player);
	msg.addByte(list.size());
	for (ChatChannel* channel : list) {
		msg.add<uint16_t>(channel->getId());
		msg.addString(channel->getName());
	}

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendChannel(uint16_t channelId, const std::string& channelName)
{
	NetworkMessage msg;
	msg.addByte(0xAC);
	msg.add<uint16_t>(channelId);
	msg.addString(channelName);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendChannelMessage(const std::string& author, const std::string& text, SpeakClasses type, uint16_t channel)
{
	NetworkMessage msg;
	msg.addByte(0xAA);
	msg.add<uint32_t>(0x00);
	msg.addString(author);
	msg.addByte(type);
	msg.add<uint16_t>(channel);
	msg.addString(text);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendIcons(uint16_t icons)
{
	NetworkMessage msg;
	msg.addByte(0xA2);
	msg.addByte(static_cast<uint8_t>(icons));
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendContainer(uint8_t cid, const Container* container, bool hasParent)
{
	NetworkMessage msg;
	msg.addByte(0x6E);
	msg.addByte(cid);
	msg.addItem(container);
	msg.addString(container->getName());
	msg.addByte(container->capacity());
	msg.addByte(hasParent ? 0x01 : 0x00);

	uint32_t containerSize = container->size();
	uint8_t itemsToSend = std::min<uint32_t>(std::min<uint32_t>(container->capacity(), containerSize), std::numeric_limits<uint8_t>::max());

	if (itemsToSend > 0) {
		msg.addByte(itemsToSend);
		for (auto it = container->getItemList().begin(), end = it + itemsToSend; it != end; ++it) {
			msg.addItem(*it);
		}
	}
	else {
		msg.addByte(0x00);
	}

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendTradeItemRequest(const std::string& traderName, const Item* item, bool ack)
{
	NetworkMessage msg;

	if (ack) {
		msg.addByte(0x7D);
	} else {
		msg.addByte(0x7E);
	}

	msg.addString(traderName);

	if (const Container* tradeContainer = item->getContainer()) {
		std::list<const Container*> listContainer {tradeContainer};
		std::list<const Item*> itemList {tradeContainer};
		while (!listContainer.empty()) {
			const Container* container = listContainer.front();
			listContainer.pop_front();

			for (Item* containerItem : container->getItemList()) {
				Container* tmpContainer = containerItem->getContainer();
				if (tmpContainer) {
					listContainer.push_back(tmpContainer);
				}
				itemList.push_back(containerItem);
			}
		}

		msg.addByte(itemList.size());
		for (const Item* listItem : itemList) {
			msg.addItem(listItem);
		}
	} else {
		msg.addByte(0x01);
		msg.addItem(item);
	}
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCloseTrade()
{
	NetworkMessage msg;
	msg.addByte(0x7F);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCloseContainer(uint8_t cid)
{
	NetworkMessage msg;
	msg.addByte(0x6F);
	msg.addByte(cid);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureTurn(const Creature* creature, uint32_t stackPos)
{
	if (!canSee(creature) || stackPos >= 10) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x6B);
	msg.addPosition(creature->getPosition());
	msg.addByte(stackPos);
	msg.add<uint16_t>(0x63);
	msg.add<uint32_t>(creature->getID());
	msg.addByte(creature->getDirection());
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureSay(uint32_t statementId, const Creature* creature, SpeakClasses type, const std::string& text, const Position* pos/* = nullptr*/)
{
	NetworkMessage msg;
	msg.addByte(0xAA);

	msg.add<uint32_t>(statementId);
	msg.addString(creature->getName());
	msg.addByte(type);

	if (pos) {
		msg.addPosition(*pos);
	} else {
		msg.addPosition(creature->getPosition());
	}

	msg.addString(text);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendToChannel(uint32_t statementId, const Creature* creature, SpeakClasses type, const std::string& text, uint16_t channelId)
{
	NetworkMessage msg;
	msg.addByte(0xAA);

	msg.add<uint32_t>(statementId);

	if (!creature) {
		msg.add<uint32_t>(0x00);
	} else {
		msg.addString(creature->getName());
	}

	msg.addByte(type);
	if (channelId == CHANNEL_RULE_REP) {
		msg.add<uint32_t>(std::time(nullptr));
	} else {
		msg.add<uint16_t>(channelId);
	}
	msg.addString(text);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendPrivateMessage(const Player* speaker, SpeakClasses type, const std::string& text)
{
	NetworkMessage msg;
	msg.addByte(0xAA);
	static uint32_t statementId = 0;
	msg.add<uint32_t>(++statementId);
	if (speaker) {
		if (type == TALKTYPE_RVR_ANSWER) {
			msg.addString("Gamemaster");
		} else {
			msg.addString(speaker->getName());
		}
	} else {
		msg.add<uint16_t>(0x00);
	}
	msg.addByte(type);
	msg.addString(text);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCancelTarget()
{
	NetworkMessage msg;
	msg.addByte(0xA3);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendChangeSpeed(const Creature* creature, uint32_t speed)
{
	NetworkMessage msg;
	msg.addByte(0x8F);
	msg.add<uint32_t>(creature->getID());
	msg.add<uint16_t>(speed);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCancelWalk()
{
	NetworkMessage msg;
	msg.addByte(0xB5);
	msg.addByte(player->getDirection());
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendSkills()
{
	NetworkMessage msg;
	AddPlayerSkills(msg);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendPing()
{
	NetworkMessage msg;
	if (player->getOperatingSystem() >= CLIENTOS_OTCLIENT_LINUX) {
		msg.addByte(0x1D);
	}
	else {
		msg.addByte(0x1E);
	}
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendPingBack()
{
	NetworkMessage msg;
	msg.addByte(0x1E);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendDistanceShoot(const Position& from, const Position& to, uint8_t type)
{
	NetworkMessage msg;
	msg.addByte(0x85);
	msg.addPosition(from);
	msg.addPosition(to);
	msg.addByte(type);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendMagicEffect(const Position& pos, uint8_t type)
{
	if (!isVisible(pos.x, pos.y, pos.z)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x83);
	msg.addPosition(pos);
	msg.addByte(type);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendCreatureHealth(const Creature* creature)
{
	NetworkMessage msg;
	msg.addByte(0x8C);
	msg.add<uint32_t>(creature->getID());

	if (creature->isHealthHidden()) {
		msg.addByte(0x00);
	} else {
		msg.addByte(std::ceil((static_cast<double>(creature->getHealth()) / std::max<int32_t>(creature->getMaxHealth(), 1)) * 100));
	}

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendFYIBox(const std::string& message)
{
	NetworkMessage msg;
	msg.addByte(0x15);
	msg.addString(message);
	writeToOutputBuffer(msg);
}

//tile
void ProtocolGame::sendMapDescription(const Position& pos)
{
	NetworkMessage msg;
	msg.addByte(0x64);
	msg.addPosition(player->getPosition());
	GetMapDescription(pos.x - Map::maxClientViewportX, pos.y - Map::maxClientViewportY, pos.z, (Map::maxClientViewportX * 2) + 2, (Map::maxClientViewportY * 2) + 2, msg);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendAddTileItem(const Position& pos, uint32_t stackpos, const Item* item)
{
	if (!canSee(pos)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x6A);
	msg.addPosition(pos);
	if (player->getOperatingSystem() >= CLIENTOS_OTCLIENT_LINUX) {
		msg.addByte(stackpos);
	}
	msg.addItem(item);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* item)
{
	if (!canSee(pos)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x6B);
	msg.addPosition(pos);
	msg.addByte(stackpos);
	msg.addItem(item);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendRemoveTileThing(const Position& pos, uint32_t stackpos)
{
	if (!canSee(pos)) {
		return;
	}

	NetworkMessage msg;
	RemoveTileThing(msg, pos, stackpos);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendUpdateTileCreature(const Position& pos, uint32_t stackpos, const Creature* creature)
{
	if (!canSee(pos)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x6B);
	msg.addPosition(pos);
	msg.addByte(stackpos);

	bool known;
	uint32_t removedKnown;
	checkCreatureAsKnown(creature->getID(), known, removedKnown);
	AddCreature(msg, creature, false, removedKnown);

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendRemoveTileCreature(const Creature*, const Position& pos, uint32_t stackpos)
{
	if (!canSee(pos) || stackpos >= 10) {
		return;
	}

	NetworkMessage msg;
	RemoveTileThing(msg, pos, stackpos);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendUpdateTile(const Tile* tile, const Position& pos)
{
	if (!canSee(pos)) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0x69);
	msg.addPosition(pos);

	if (tile) {
		GetTileDescription(tile, msg);
		msg.addByte(0x00);
		msg.addByte(0xFF);
	} else {
		msg.addByte(0x01);
		msg.addByte(0xFF);
	}

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendFightModes()
{
	NetworkMessage msg;
	msg.addByte(0xA7);
	msg.addByte(player->fightMode);
	msg.addByte(player->chaseMode);
	msg.addByte(player->secureMode);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendAddCreature(const Creature* creature, const Position& pos, int32_t stackpos)
{
	if (!canSee(pos)) {
		return;
	}

	if (creature != player) {
		// stack pos is always real index now, so it can exceed the limit
		// if stack pos exceeds the limit, we need to refresh the tile instead
		// 1. this is a rare case, and is only triggered by forcing summon in a position
		// 2. since no stackpos will be send to the client about that creature, removing
		//    it must be done with its id if its stackpos remains >= 10. this is done to
		//    add creatures to battle list instead of rendering on screen
		if (stackpos >= 10) {
			// @todo: should we avoid this check?
			if (const Tile* tile = creature->getTile()) {
				sendUpdateTile(tile, pos);
			}
		} else {
			// if stackpos is -1, the client will automatically detect it
			NetworkMessage msg;
			msg.addByte(0x6A);
			msg.addPosition(pos);

			if (player->getOperatingSystem() >= CLIENTOS_OTCLIENT_LINUX) {
				msg.addByte(stackpos);
			}

			bool known;
			uint32_t removedKnown;
			checkCreatureAsKnown(creature->getID(), known, removedKnown);
			AddCreature(msg, creature, known, removedKnown);
			writeToOutputBuffer(msg);
		}

		return;
	}

	NetworkMessage msg;
	msg.addByte(0x0A);
	msg.add<uint32_t>(player->getID());
	msg.add<uint16_t>(0x32); // beat duration (50)

	// can report bugs?
	if (player->getAccountType() >= ACCOUNT_TYPE_TUTOR) {
		msg.addByte(0x01);
	} else {
		msg.addByte(0x00);
	}

	writeToOutputBuffer(msg);

	NetworkMessage rightsMsg;

	bool sendRights = false;

	if (!player->getGroup()->ruleViolationRights.empty()) {
		rightsMsg.addByte(0x0B);

		for (int32_t right = 18; right <= 49; right++) {
			int32_t flag = 0;

			if (player->getGroup()->ruleViolationRights.contains(right)) {
				for (int32_t i = 0; i <= 6; i++) {
					bool pass = false;
					if (i == 0)
						pass = player->getGroup()->ruleViolationRights.contains(NOTATION);
					else if (i == 1)
						pass = player->getGroup()->ruleViolationRights.contains(NAMELOCK);
					else if (i == 2)
						pass = player->getGroup()->ruleViolationRights.contains(BANISHMENT);
					else if (i == 3)
						pass = player->getGroup()->ruleViolationRights.contains(NAMELOCK) && player->getGroup()->ruleViolationRights.contains(BANISHMENT);
					else if (i == 4)
						pass = player->getGroup()->ruleViolationRights.contains(BANISHMENT) && player->getGroup()->ruleViolationRights.contains(FINAL_WARNING);
					else if (i == 5)
						pass = player->getGroup()->ruleViolationRights.contains(NAMELOCK) && player->getGroup()->ruleViolationRights.contains(BANISHMENT) && player->getGroup()->ruleViolationRights.contains(FINAL_WARNING);
					else if (i == 6) {
						if ((right >= STATEMENT_INSULTING && right <= GAMEMASTER_FALSE_REPORTS) || right == STATEMENT_ADVERT_OFFTOPIC) {
							pass = player->getGroup()->ruleViolationRights.contains(STATEMENT_REPORT);
						}
					}

					if (pass)
						flag |= 1 << i;
				}

				if (flag && player->getGroup()->ruleViolationRights.contains(IP_BANISHMENT))
					flag |= 0x80;
			}

			rightsMsg.addByte(flag);

			if (flag)
				sendRights = true;
		}
	}

	if (sendRights)
		writeToOutputBuffer(rightsMsg);

	sendMapDescription(pos);

	for (int i = CONST_SLOT_FIRST; i <= CONST_SLOT_LAST; ++i) {
		sendInventoryItem(static_cast<slots_t>(i), player->getInventoryItem(static_cast<slots_t>(i)));
	}

	sendStats();
	sendSkills();

	//gameworld light-settings*/
	sendWorldLight(g_game.getWorldLightInfo());

	//player light level
	sendCreatureLight(creature);

	sendVIPEntries();

	player->sendIcons();
}

void ProtocolGame::sendMoveCreature(const Creature* creature, const Position& newPos, int32_t newStackPos, const Position& oldPos, int32_t oldStackPos, bool teleport)
{
	if (creature == player) {
		if (teleport || oldStackPos >= 10) {
			sendRemoveTileCreature(creature, oldPos, oldStackPos);
			if (newPos.z != 8 && oldPos.z != 7) {
				if (oldStackPos >= 10 && canSee(newPos) && canSee(oldPos)) {
					sendUpdateTile(g_game.map.getTile(oldPos), oldPos);
					NetworkMessage msg;

					msg.addByte(0x6A);
					msg.addPosition(newPos);

					if (player->getOperatingSystem() >= CLIENTOS_OTCLIENT_LINUX) {
						msg.addByte(newStackPos);
					}

					bool known;
					uint32_t removedKnown;
					checkCreatureAsKnown(creature->getID(), known, removedKnown);
					AddCreature(msg, creature, known, removedKnown);
					writeToOutputBuffer(msg);
				}
			}
			sendMapDescription(newPos);
		} else {
			NetworkMessage msg;
			if (oldPos.z == 7 && newPos.z >= 8) {
				RemoveTileCreature(msg, creature, oldPos, oldStackPos);
			} else {
				msg.addByte(0x6D);
				if (oldStackPos < 10) {
					msg.addPosition(oldPos);
					msg.addByte(oldStackPos);
				} else {
					msg.add<uint16_t>(0xFFFF);
					msg.add<uint32_t>(creature->getID());
				}
				msg.addPosition(newPos);
			}

			if (newPos.z > oldPos.z) {
				MoveDownCreature(msg, creature, newPos, oldPos);
			} else if (newPos.z < oldPos.z) {
				MoveUpCreature(msg, creature, newPos, oldPos);
			}

			if (oldPos.y > newPos.y) { // north, for old x
				msg.addByte(0x65);
				GetMapDescription(oldPos.x - Map::maxClientViewportX, newPos.y - Map::maxClientViewportY, newPos.z, (Map::maxClientViewportX * 2) + 2, 1, msg);
			} else if (oldPos.y < newPos.y) { // south, for old x
				msg.addByte(0x67);
				GetMapDescription(oldPos.x - Map::maxClientViewportX, newPos.y + (Map::maxClientViewportY + 1), newPos.z, (Map::maxClientViewportX * 2) + 2, 1, msg);
			}

			if (oldPos.x < newPos.x) { // east, [with new y]
				msg.addByte(0x66);
				GetMapDescription(newPos.x + (Map::maxClientViewportX + 1), newPos.y - Map::maxClientViewportY, newPos.z, 1, (Map::maxClientViewportY * 2) + 2, msg);
			} else if (oldPos.x > newPos.x) { // west, [with new y]
				msg.addByte(0x68);
				GetMapDescription(newPos.x - Map::maxClientViewportX, newPos.y - Map::maxClientViewportY, newPos.z, 1, (Map::maxClientViewportY * 2) + 2, msg);
			}
			writeToOutputBuffer(msg);
		}
	} else if (canSee(oldPos) && canSee(creature->getPosition())) {
		if (teleport || (oldPos.z == 7 && newPos.z >= 8) || oldStackPos >= 10) {
			sendRemoveTileCreature(creature, oldPos, oldStackPos);
			if (oldStackPos >= 10) {
				sendUpdateTile(g_game.map.getTile(oldPos), oldPos);
			}
			sendAddCreature(creature, newPos, newStackPos);
		} else {
			NetworkMessage msg;
			msg.addByte(0x6D);
			if (oldStackPos < 10) {
				msg.addPosition(oldPos);
				msg.addByte(oldStackPos);
			} else {
				msg.add<uint16_t>(0xFFFF);
				msg.add<uint32_t>(creature->getID());
			}
			msg.addPosition(creature->getPosition());
			writeToOutputBuffer(msg);
		}
	} else if (canSee(oldPos)) {
		sendRemoveTileCreature(creature, oldPos, oldStackPos);
	} else if (canSee(creature->getPosition())) {
		sendAddCreature(creature, newPos, newStackPos);
	}
}

void ProtocolGame::sendInventoryItem(slots_t slot, const Item* item)
{
	NetworkMessage msg;
	if (item) {
		msg.addByte(0x78);
		msg.addByte(slot);
		msg.addItem(item);
	} else {
		msg.addByte(0x79);
		msg.addByte(slot);
	}
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendAddContainerItem(uint8_t cid, const Item* item)
{
	NetworkMessage msg;
	msg.addByte(0x70);
	msg.addByte(cid);
	msg.addItem(item);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendUpdateContainerItem(uint8_t cid, uint16_t slot, const Item* item)
{
	NetworkMessage msg;
	msg.addByte(0x71);
	msg.addByte(cid);
	msg.addByte(static_cast<uint8_t>(slot));
	msg.addItem(item);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendRemoveContainerItem(uint8_t cid, uint16_t slot)
{
	NetworkMessage msg;
	msg.addByte(0x72);
	msg.addByte(cid);
	msg.addByte(static_cast<uint8_t>(slot));
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendTextWindow(uint32_t windowTextId, Item* item, uint16_t maxlen, bool canWrite)
{
	NetworkMessage msg;
	msg.addByte(0x96);
	msg.add<uint32_t>(windowTextId);
	msg.addItem(item);

	if (canWrite) {
		msg.add<uint16_t>(maxlen);
		msg.addString(item->getText());
	} else {
		const std::string& text = item->getText();
		msg.add<uint16_t>(text.size());
		msg.addString(text);
	}

	const std::string& writer = item->getWriter();
	if (!writer.empty()) {
		msg.addString(writer);
	} else {
		msg.add<uint16_t>(0x00);
	}

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendTextWindow(uint32_t windowTextId, uint32_t itemId, const std::string& text)
{
	NetworkMessage msg;
	msg.addByte(0x96);
	msg.add<uint32_t>(windowTextId);
	msg.addItem(itemId, 1);
	msg.add<uint16_t>(text.size());
	msg.addString(text);
	msg.add<uint16_t>(0x00);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendHouseWindow(uint32_t windowTextId, const std::string& text)
{
	NetworkMessage msg;
	msg.addByte(0x97);
	msg.addByte(0x00);
	msg.add<uint32_t>(windowTextId);
	msg.addString(text);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendOutfitWindow()
{
	const auto& outfits = Outfits::getInstance().getOutfits(player->getSex());
	if (outfits.size() == 0) {
		return;
	}

	NetworkMessage msg;
	msg.addByte(0xC8);

	if (player->getOperatingSystem() >= CLIENTOS_OTCLIENT_LINUX) {
		Outfit_t currentOutfit = player->getDefaultOutfit();
		if (currentOutfit.lookType == 0) {
			Outfit_t newOutfit;
			newOutfit.lookType = outfits.front().lookType;
			currentOutfit = newOutfit;
		}

		AddOutfit(msg, currentOutfit);

		std::vector<ProtocolOutfit> protocolOutfits;
		if (player->isAccessPlayer()) {
			static const std::string gamemasterOutfitName = "Gamemaster";
			protocolOutfits.emplace_back(gamemasterOutfitName, 75);
		}

		protocolOutfits.reserve(outfits.size());
		for (const Outfit& outfit : outfits) {
			protocolOutfits.emplace_back(outfit.name, outfit.lookType);
			if (protocolOutfits.size() == std::numeric_limits<uint8_t>::max()) { // Game client currently doesn't allow more than 255 outfits
				break;
			}
		}

		msg.addByte(protocolOutfits.size());
		for (const ProtocolOutfit& outfit : protocolOutfits) {
			msg.add<uint16_t>(outfit.lookType);
			msg.addString(outfit.name);
		}
	} else {
		Outfit_t currentOutfit = player->getDefaultOutfit();
		AddOutfit(msg, currentOutfit);

		if (player->getSex() == PLAYERSEX_MALE) {
			msg.add<uint16_t>(128);
			if (player->isPremium()) {
				msg.add<uint16_t>(134);
			} else {
				msg.add<uint16_t>(131);
			}
		} else {
			msg.add<uint16_t>(136);
			if (player->isPremium()) {
				msg.add<uint16_t>(142);
			} else {
				msg.add<uint16_t>(139);
			}
		}
	}

	writeToOutputBuffer(msg);
}

void ProtocolGame::sendUpdatedVIPStatus(uint32_t guid, VipStatus_t newStatus)
{
	NetworkMessage msg;
	if (newStatus == VIPSTATUS_ONLINE) {
		msg.addByte(0xD3);
	}
	else {
		msg.addByte(0xD4);
	}
	msg.add<uint32_t>(guid);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendVIP(uint32_t guid, const std::string& name, VipStatus_t status)
{
	NetworkMessage msg;
	msg.addByte(0xD2);
	msg.add<uint32_t>(guid);
	msg.addString(name);
	msg.addByte(status);
	writeToOutputBuffer(msg);
}

void ProtocolGame::sendVIPEntries()
{
	const auto& vipEntries = player->VIPList;
	for (const uint32_t& entry : vipEntries) {
		VipStatus_t vipStatus = VIPSTATUS_ONLINE;

		const Player* vipPlayer = g_game.getPlayerByGUID(entry);

		if (!vipPlayer || !player->canSeeCreature(vipPlayer)) {
			vipStatus = VIPSTATUS_OFFLINE;
		}

		if (vipPlayer) {
			sendVIP(entry, vipPlayer->getName(), vipStatus);
		} else {
			sendVIP(entry, g_game.getStoredPlayerNameByGUID(entry), vipStatus);
		}
	}
}

void ProtocolGame::sendModalWindow(const ModalWindow& modalWindow)
{
	NetworkMessage msg;
	msg.addByte(0xFA);

	msg.add<uint32_t>(modalWindow.id);
	msg.addString(modalWindow.title);
	msg.addString(modalWindow.message);

	msg.addByte(modalWindow.buttons.size());
	for (const auto& it : modalWindow.buttons) {
		msg.addString(it.first);
		msg.addByte(it.second);
	}

	msg.addByte(modalWindow.choices.size());
	for (const auto& it : modalWindow.choices) {
		msg.addString(it.first);
		msg.addByte(it.second);
	}

	msg.addByte(modalWindow.defaultEscapeButton);
	msg.addByte(modalWindow.defaultEnterButton);
	msg.addByte(modalWindow.priority ? 0x01 : 0x00);

	writeToOutputBuffer(msg);
}

////////////// Add common messages
void ProtocolGame::AddCreature(NetworkMessage& msg, const Creature* creature, bool known, uint32_t remove)
{
	const Player* otherPlayer = creature->getPlayer();

	if (known) {
		msg.add<uint16_t>(0x62);
		msg.add<uint32_t>(creature->getID());
	} else {
		msg.add<uint16_t>(0x61);
		msg.add<uint32_t>(remove);
		msg.add<uint32_t>(creature->getID());
		msg.addString(creature->getName());
	}

	if (creature->isHealthHidden()) {
		msg.addByte(0x00);
	} else {
		msg.addByte(std::ceil((static_cast<double>(creature->getHealth()) / std::max<int32_t>(creature->getMaxHealth(), 1)) * 100));
	}

	msg.addByte(creature->getDirection());

	if (!creature->isInGhostMode() && !creature->isInvisible()) {
		AddOutfit(msg, creature->getCurrentOutfit());
	} else {
		static Outfit_t outfit;
		AddOutfit(msg, outfit);
	}

	LightInfo lightInfo = creature->getCreatureLight();
	msg.addByte(lightInfo.level);
	msg.addByte(lightInfo.color);

	msg.add<uint16_t>(creature->getStepSpeed());

	msg.addByte(player->getSkullClient(creature));
	msg.addByte(player->getPartyShield(otherPlayer));
}

void ProtocolGame::AddPlayerStats(NetworkMessage& msg)
{
	msg.addByte(0xA0);

	msg.add<uint16_t>(std::min<int32_t>(player->getHealth(), std::numeric_limits<uint16_t>::max()));
	msg.add<uint16_t>(std::min<int32_t>(player->getMaxHealth(), std::numeric_limits<uint16_t>::max()));
	
	if (player->hasFlag(PlayerFlag_HasInfiniteCapacity)) {
		// This has to be done here, because getFreeCapacity handles inventory space.
		msg.add<uint16_t>(0);
	} else {
		msg.add<uint16_t>(static_cast<uint16_t>(player->getFreeCapacity() / 100.));
	}

	if (player->getExperience() >= std::numeric_limits<uint32_t>::max() - 1) {
		msg.add<uint32_t>(0);
	}
	else {
		msg.add<uint32_t>(static_cast<uint32_t>(player->getExperience()));
	}

	msg.add<uint16_t>(static_cast<uint16_t>(player->getLevel()));
	msg.addByte(player->getLevelPercent());

	msg.add<uint16_t>(std::min<int32_t>(player->getMana(), std::numeric_limits<uint16_t>::max()));
	msg.add<uint16_t>(std::min<int32_t>(player->getMaxMana(), std::numeric_limits<uint16_t>::max()));

	msg.addByte(std::min<uint32_t>(player->getMagicLevel(), std::numeric_limits<uint8_t>::max()));
	msg.addByte(player->getMagicLevelPercent());

	msg.addByte(player->getSoul());
}

void ProtocolGame::AddPlayerSkills(NetworkMessage& msg)
{
	msg.addByte(0xA1);

	for (uint8_t i = SKILL_FIRST; i <= SKILL_LAST; ++i) {
		msg.addByte(std::min<int32_t>(player->getSkillLevel(i), std::numeric_limits<uint16_t>::max()));
		msg.addByte(player->getSkillPercent(i));
	}
}

void ProtocolGame::AddOutfit(NetworkMessage& msg, const Outfit_t& outfit)
{
	msg.add<uint16_t>(outfit.lookType);

	if (outfit.lookType != 0) {
		msg.addByte(outfit.lookHead);
		msg.addByte(outfit.lookBody);
		msg.addByte(outfit.lookLegs);
		msg.addByte(outfit.lookFeet);
	} else {
		msg.addItemId(outfit.lookTypeEx);
	}
}

void ProtocolGame::AddWorldLight(NetworkMessage& msg, LightInfo lightInfo)
{
	msg.addByte(0x82);
	msg.addByte(lightInfo.level);
	msg.addByte(lightInfo.color);
}

void ProtocolGame::AddCreatureLight(NetworkMessage& msg, const Creature* creature)
{
	LightInfo lightInfo = creature->getCreatureLight();

	msg.addByte(0x8D);
	msg.add<uint32_t>(creature->getID());
	msg.addByte(lightInfo.level);
	msg.addByte(lightInfo.color);
}

//tile
void ProtocolGame::RemoveTileThing(NetworkMessage& msg, const Position& pos, uint32_t stackpos)
{
	if (stackpos >= 10) {
		return;
	}

	msg.addByte(0x6C);
	msg.addPosition(pos);
	msg.addByte(stackpos);
}

void ProtocolGame::RemoveTileCreature(NetworkMessage& msg, const Creature*, const Position& pos, uint32_t stackpos)
{
	if (stackpos >= 10) {
		return;
	}

	RemoveTileThing(msg, pos, stackpos);
}

void ProtocolGame::MoveUpCreature(NetworkMessage& msg, const Creature* creature, const Position& newPos, const Position& oldPos)
{
	if (creature != player) {
		return;
	}

	//floor change up
	msg.addByte(0xBE);

	//going to surface
	if (newPos.z == 7) {
		int32_t skip = -1;

		// floor 7 and 6 already set
		for (int i = 5; i >= 0; --i) {
			GetFloorDescription(msg, oldPos.x - Map::maxClientViewportX, oldPos.y - Map::maxClientViewportY, i, (Map::maxClientViewportX * 2) + 2, (Map::maxClientViewportY * 2) + 2, 8 - i, skip);
		}
		if (skip >= 0) {
			msg.addByte(skip);
			msg.addByte(0xFF);
		}
	}
	//underground, going one floor up (still underground)
	else if (newPos.z > 7) {
		int32_t skip = -1;
		GetFloorDescription(msg, oldPos.x - Map::maxClientViewportX, oldPos.y - Map::maxClientViewportY, oldPos.getZ() - 3, (Map::maxClientViewportX * 2) + 2, (Map::maxClientViewportY * 2) + 2, 3, skip);

		if (skip >= 0) {
			msg.addByte(skip);
			msg.addByte(0xFF);
		}
	}

	//moving up a floor up makes us out of sync
	//west
	msg.addByte(0x68);
	GetMapDescription(oldPos.x - Map::maxClientViewportX, oldPos.y - (Map::maxClientViewportY - 1), newPos.z, 1, (Map::maxClientViewportY * 2) + 2, msg);

	//north
	msg.addByte(0x65);
	GetMapDescription(oldPos.x - Map::maxClientViewportX, oldPos.y - Map::maxClientViewportY, newPos.z, (Map::maxClientViewportX * 2) + 2, 1, msg);
}

void ProtocolGame::MoveDownCreature(NetworkMessage& msg, const Creature* creature, const Position& newPos, const Position& oldPos)
{
	if (creature != player) {
		return;
	}

	//floor change down
	msg.addByte(0xBF);

	//going from surface to underground
	if (newPos.z == 8) {
		int32_t skip = -1;

		for (int i = 0; i < 3; ++i) {
			GetFloorDescription(msg, oldPos.x - Map::maxClientViewportX, oldPos.y - Map::maxClientViewportY, newPos.z + i, (Map::maxClientViewportX * 2) + 2, (Map::maxClientViewportY * 2) + 2, -i - 1, skip);
		}
		if (skip >= 0) {
			msg.addByte(skip);
			msg.addByte(0xFF);
		}
	}
	//going further down
	else if (newPos.z > oldPos.z && newPos.z > 8 && newPos.z < 14) {
		int32_t skip = -1;
		GetFloorDescription(msg, oldPos.x - Map::maxClientViewportX, oldPos.y - Map::maxClientViewportY, newPos.z + 2, (Map::maxClientViewportX * 2) + 2, (Map::maxClientViewportY * 2) + 2, -3, skip);

		if (skip >= 0) {
			msg.addByte(skip);
			msg.addByte(0xFF);
		}
	}

	//moving down a floor makes us out of sync
	//east
	msg.addByte(0x66);
	GetMapDescription(oldPos.x + (Map::maxClientViewportX + 1), oldPos.y - (Map::maxClientViewportY + 1), newPos.z, 1, (Map::maxClientViewportY * 2) + 2, msg);

	//south
	msg.addByte(0x67);
	GetMapDescription(oldPos.x - Map::maxClientViewportX, oldPos.y + (Map::maxClientViewportY + 1), newPos.z, (Map::maxClientViewportX * 2) + 2, 1, msg);
}

void ProtocolGame::parseExtendedOpcode(NetworkMessage& msg)
{
	uint8_t opcode = msg.getByte();
	const std::string& buffer = msg.getString();

	// process additional opcodes via lua script event
	g_game.parsePlayerExtendedOpcode(player->getID(), opcode, buffer);
}
