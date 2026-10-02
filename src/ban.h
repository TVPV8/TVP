// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

struct BanInfo
{
	std::string bannedBy;
	std::string reason;
	time_t expiresAt;
};

struct ConnectBlock
{
	constexpr ConnectBlock(uint64_t lastAttempt, uint64_t blockTime, uint32_t count) :
	    lastAttempt(lastAttempt), blockTime(blockTime), count(count)
	{}

	uint64_t lastAttempt;
	uint64_t blockTime;
	uint32_t count;
};

using IpConnectMap = std::map<uint32_t, ConnectBlock>;

class Ban
{
public:
	bool acceptConnection(uint32_t clientIP);

private:
	IpConnectMap ipConnectMap;
	std::recursive_mutex lock;
};

class IOBan
{
public:
	static bool isAccountBanned(uint32_t accountId, BanInfo& banInfo);
	static bool isIpBanned(uint32_t clientIP, BanInfo& banInfo);
	static bool isPlayerNamelocked(uint32_t playerId);
};
