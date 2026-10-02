// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

class Guild;
using GuildWarVector = std::vector<uint32_t>;

class IOGuild
{
public:
	static Guild* loadGuild(uint32_t guildId);
	static uint32_t getGuildIdByName(const std::string& name);
	static void getWarList(uint32_t guildId, GuildWarVector& guildWarVector);
};
