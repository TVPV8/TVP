// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include "player.h"
#include "monsters.h"

class Player;
class Party;

using PlayerVector = std::vector<Player*>;

class Party
{
public:
	explicit Party(Player* leader);

	Player* getLeader() const { return leader; }
	PlayerVector& getMembers() { return memberList; }
	const std::vector<uint32_t>& getInvitees() const { return inviteList; }
	size_t getMemberCount() const { return memberList.size(); }
	size_t getInvitationCount() const { return inviteList.size(); }

	void disband();
	bool invitePlayer(Player& player);
	bool joinParty(Player& player);
	void revokeInvitation(Player& player);
	bool passPartyLeadership(Player* player);
	bool leaveParty(Player* player, bool forceRemove = false);

	bool removeInvite(Player& player);

	bool isPlayerInvited(const Player* player) const;
	void updateAllPartyIcons();
	void broadcastPartyMessage(MessageClasses msgClass, const std::string& msg, bool sendToInvitations = false);
	bool empty() const { return memberList.empty() && inviteList.empty(); }
	bool canOpenCorpse(uint32_t ownerId) const;

	void shareExperience(uint64_t experience, Creature* source = nullptr);
	bool setSharedExperience(Player* player, bool sharedExpActive);
	bool isSharedExperienceActive() const { return sharedExpActive; }
	bool isSharedExperienceEnabled() const { return sharedExpEnabled; }
	bool canUseSharedExperience(const Player* player) const;
	void updateSharedExperience();

	void updatePlayerTicks(Player* player, uint32_t points);
	void clearPlayerPoints(Player* player);

private:
	bool canEnableSharedExperience();

	std::map<uint32_t, int64_t> ticksMap;

	PlayerVector memberList;
	std::vector<uint32_t> inviteList;

	Player* leader;

	bool sharedExpActive = false;
	bool sharedExpEnabled = false;
};
