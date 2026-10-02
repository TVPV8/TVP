// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "protocollogin.h"

#include "outputmessage.h"
#include "tasks.h"

#include "configmanager.h"
#include "iologindata.h"
#include "ban.h"
#include "game.h"

#include <fmt/format.h>

extern ConfigManager g_config;
extern Game g_game;

void ProtocolLogin::disconnectClient(const std::string& message)
{
	auto output = OutputMessagePool::getOutputMessage();

	output->addByte(0x0A);
	output->addString(message);
	send(output);

	disconnect();
}

void ProtocolLogin::getCharacterList(uint32_t accountNumber, const std::string& password)
{
	Account account;
	if (!IOLoginData::loginserverAuthentication(accountNumber, password, account)) {
		g_game.registerFailedIPLogin(getIP());
		g_game.registerFailedAccountLogin(accountNumber);
		disconnectClient("Account number or password is not correct.");
		return;
	}

	g_game.resetIpLoginAttempts(getIP());
	g_game.resetAccountLoginAttempts(accountNumber);

	auto output = OutputMessagePool::getOutputMessage();

	const std::string& motd = g_config.getString(ConfigManager::MOTD);
	if (!motd.empty()) {
		// Add MOTD
		output->addByte(0x14);
		output->addString(fmt::format("{:d}\n{:s}", g_game.getMotdNum(), motd));
	}

	// Add char list
	output->addByte(0x64);

	uint8_t size = std::min<size_t>(std::numeric_limits<uint8_t>::max(), account.characters.size());

	output->addByte(size);
	for (uint8_t i = 0; i < size; i++) {
		const std::string& character = account.characters[i];
		output->addString(character);
		output->addString(g_config.getString(ConfigManager::SERVER_NAME));
		output->add<uint32_t>(inet_addr(g_config.getString(ConfigManager::IP).c_str()));
		output->add<uint16_t>(g_config.getNumber(ConfigManager::GAME_PORT));
	}

	// Add premium days
	if (g_config.getBoolean(ConfigManager::FREE_PREMIUM)) {
		output->add<uint16_t>(std::numeric_limits<uint16_t>::max());
	} else {
		if (account.premiumEndsAt > std::time(nullptr)) {
			int32_t premiumDaysLeft =
			    std::floor<uint16_t>(std::max<int32_t>(0, account.premiumEndsAt - std::time(nullptr)) / 86400);
			output->add<uint16_t>(++premiumDaysLeft);
		} else {
			output->add<uint16_t>(0);
		}
	}

	send(output);

	disconnect();
}

void ProtocolLogin::onRecvFirstMessage(NetworkMessage& msg)
{
	if (g_game.getGameState() == GAME_STATE_SHUTDOWN) {
		disconnect();
		return;
	}

	msg.skipBytes(2); // client OS
	uint16_t version = msg.get<uint16_t>();

	msg.skipBytes(12);

	/*
	 * Skipped bytes:
	 * 4 bytes: protocolVersion
	 * 12 bytes: dat, spr, pic signatures (4 bytes each)
	 */

	if (version <= 760) {
		disconnectClient(fmt::format("Only clients with protocol {:s} allowed!", CLIENT_VERSION_STR));
		return;
	}

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

	if (version < CLIENT_VERSION_MIN || version > CLIENT_VERSION_MAX) {
		disconnectClient(fmt::format("Only clients with protocol {:s} allowed!", CLIENT_VERSION_STR));
		return;
	}

	if (g_game.getGameState() == GAME_STATE_STARTUP) {
		disconnectClient("Gameworld is starting up. Please wait.");
		return;
	}

	if (g_game.getGameState() == GAME_STATE_MAINTAIN) {
		disconnectClient("Gameworld is under maintenance.\nPlease re-connect in a while.");
		return;
	}

	BanInfo banInfo;
	auto connection = getConnection();
	if (!connection) {
		return;
	}

	if (IOBan::isIpBanned(connection->getIP(), banInfo)) {
		if (banInfo.reason.empty()) {
			banInfo.reason = "(none)";
		}

		disconnectClient(fmt::format("Your IP has been banned until {:s} by {:s}.\n\nReason specified:\n{:s}",
		                             formatDateShort(banInfo.expiresAt), banInfo.bannedBy, banInfo.reason));
		return;
	}

	uint32_t accountNumber = msg.get<uint32_t>();
	if (accountNumber == 0) {
		disconnectClient("Invalid account number.");
		return;
	}

	std::string password = msg.getString();
	if (password.empty()) {
		disconnectClient("Invalid password.");
		return;
	}

	if (g_game.isAccountLocked(accountNumber)) {
		disconnectClient(g_config.getString(ConfigManager::ACCOUNT_LOCK_MESSAGE));
		return;
	}

	auto thisPtr = std::static_pointer_cast<ProtocolLogin>(shared_from_this());
	g_dispatcher.addTask(createTask(std::bind(&ProtocolLogin::getCharacterList, thisPtr, accountNumber, password)));
}
