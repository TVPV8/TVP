// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

struct Group
{
	std::string name;
	uint64_t flags;
	uint32_t maxDepotItems;
	uint32_t maxVipEntries;
	uint16_t id;
	std::set<int32_t> ruleViolationRights;
	bool access;
};

class Groups
{
public:
	bool load();
	Group* getGroup(uint16_t id);

private:
	std::vector<Group> groups;
};
