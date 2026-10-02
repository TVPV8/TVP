// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include "account.h"
#include "player.h"

class IOLoginData
{
public:
	static Account loadAccount(uint32_t accno);

	static bool loginserverAuthentication(uint32_t accountNumber, const std::string& password, Account& account);
	static uint32_t gameworldAuthentication(uint32_t accountNumber, const std::string& password,
	                                        std::string& characterName);

	static uint32_t getAccountIdByPlayerName(const std::string& playerName);
	static uint32_t getAccountIdByPlayerId(uint32_t playerId);

	static AccountType_t getAccountType(uint32_t accountId);
	static void setAccountType(uint32_t accountId, AccountType_t accountType);
	static void updateOnlineStatus(uint32_t guid, bool login);
	static bool preloadPlayer(Player* player, const std::string& name);

	static bool loadPlayerByGUID(Player* player, uint32_t id);
	static bool loadPlayerByName(Player* player, const std::string& name);
	static bool loadPlayer(Player* player, bool initializeScriptFile);
	static bool savePlayer(Player* player);

	static uint32_t getGuidByName(const std::string& name);
	static bool getGuidByNameEx(uint32_t& guid, bool& specialVip, std::string& name);
	static std::string getNameByGuid(uint32_t guid);
	static bool formatPlayerName(std::string& name);

	static void increaseBankBalance(uint32_t guid, uint64_t bankBalance);
	static bool hasBiddedOnHouse(uint32_t guid);

	static void updatePremiumTime(uint32_t accountId, time_t endTime);
};
