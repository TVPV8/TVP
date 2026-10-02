// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "groups.h"

#include "pugicast.h"
#include "tools.h"

const std::unordered_map<std::string, PlayerFlags> ParsePlayerFlagMap = {
    {"cannotusecombat", PlayerFlag_CannotUseCombat},
    {"cannotattackplayer", PlayerFlag_CannotAttackPlayer},
    {"cannotattackmonster", PlayerFlag_CannotAttackMonster},
    {"cannotbeattacked", PlayerFlag_CannotBeAttacked},
    {"canconvinceall", PlayerFlag_CanConvinceAll},
    {"cansummonall", PlayerFlag_CanSummonAll},
    {"canillusionall", PlayerFlag_CanIllusionAll},
    {"cansenseinvisibility", PlayerFlag_CanSenseInvisibility},
    {"ignoredbymonsters", PlayerFlag_IgnoredByMonsters},
    {"hasinfinitemana", PlayerFlag_HasInfiniteMana},
    {"hasinfinitesoul", PlayerFlag_HasInfiniteSoul},
    {"hasnoexhaustion", PlayerFlag_HasNoExhaustion},
    {"cannotusespells", PlayerFlag_CannotUseSpells},
    {"cannotpickupitem", PlayerFlag_CannotPickupItem},
    {"canalwayslogin", PlayerFlag_CanAlwaysLogin},
    {"canbroadcast", PlayerFlag_CanBroadcast},
    {"canedithouses", PlayerFlag_CanEditHouses},
    {"cannotbebanned", PlayerFlag_CannotBeBanned},
    {"cannotbepushed", PlayerFlag_CannotBePushed},
    {"hasinfinitecapacity", PlayerFlag_HasInfiniteCapacity},
    {"canpushallcreatures", PlayerFlag_CanPushAllCreatures},
    {"cantalkredprivate", PlayerFlag_CanTalkRedPrivate},
    {"cantalkredchannel", PlayerFlag_CanTalkRedChannel},
    {"talkorangehelpchannel", PlayerFlag_TalkOrangeHelpChannel},
    {"notgainexperience", PlayerFlag_NotGainExperience},
    {"notgainmana", PlayerFlag_NotGainMana},
    {"notgainhealth", PlayerFlag_NotGainHealth},
    {"notgainskill", PlayerFlag_NotGainSkill},
    {"setmaxspeed", PlayerFlag_SetMaxSpeed},
    {"specialvip", PlayerFlag_SpecialVIP},
    {"notgenerateloot", PlayerFlag_NotGenerateLoot},
    {"ignoreprotectionzone", PlayerFlag_IgnoreProtectionZone},
    {"ignorespellcheck", PlayerFlag_IgnoreSpellCheck},
    {"ignoreweaponcheck", PlayerFlag_IgnoreWeaponCheck},
    {"cannotbemuted", PlayerFlag_CannotBeMuted},
    {"isalwayspremium", PlayerFlag_IsAlwaysPremium},
    {"fulllight", PlayerFlag_FullLight}};

const std::unordered_map<std::string, RuleViolationRights> ParseRuleViolationRightsMap = {
    {"NOTATION", RuleViolationRights::NOTATION},
    {"NAMELOCK", RuleViolationRights::NAMELOCK},
    {"STATEMENT_REPORT", RuleViolationRights::STATEMENT_REPORT},
    {"BANISHMENT", RuleViolationRights::BANISHMENT},
    {"FINAL_WARNING", RuleViolationRights::FINAL_WARNING},
    {"IP_BANISHMENT", RuleViolationRights::IP_BANISHMENT},
    {"NAME_INSULTING", RuleViolationRights::NAME_INSULTING},
    {"NAME_SENTENCE", RuleViolationRights::NAME_SENTENCE},
    {"NAME_NONSENSICAL_LETTERS", RuleViolationRights::NAME_NONSENSICAL_LETTERS},
    {"NAME_BADLY_FORMATTED", RuleViolationRights::NAME_BADLY_FORMATTED},
    {"NAME_NO_PERSON", RuleViolationRights::NAME_NO_PERSON},
    {"NAME_CELEBRITY", RuleViolationRights::NAME_CELEBRITY},
    {"NAME_COUNTRY", RuleViolationRights::NAME_COUNTRY},
    {"NAME_FAKE_IDENTITY", RuleViolationRights::NAME_FAKE_IDENTITY},
    {"NAME_FAKE_POSITION", RuleViolationRights::NAME_FAKE_POSITION},
    {"STATEMENT_INSULTING", RuleViolationRights::STATEMENT_INSULTING},
    {"STATEMENT_SPAMMING", RuleViolationRights::STATEMENT_SPAMMING},
    {"STATEMENT_ADVERT_OFFTOPIC", RuleViolationRights::STATEMENT_ADVERT_OFFTOPIC},
    {"STATEMENT_ADVERT_MONEY", RuleViolationRights::STATEMENT_ADVERT_MONEY},
    {"STATEMENT_NON_ENGLISH", RuleViolationRights::STATEMENT_NON_ENGLISH},
    {"STATEMENT_CHANNEL_OFFTOPIC", RuleViolationRights::STATEMENT_CHANNEL_OFFTOPIC},
    {"STATEMENT_VIOLATION_INCITING", RuleViolationRights::STATEMENT_VIOLATION_INCITING},
    {"CHEATING_BUG_ABUSE", RuleViolationRights::CHEATING_BUG_ABUSE},
    {"CHEATING_GAME_WEAKNESS", RuleViolationRights::CHEATING_GAME_WEAKNESS},
    {"CHEATING_MACRO_USE", RuleViolationRights::CHEATING_MACRO_USE},
    {"CHEATING_MODIFIED_CLIENT", RuleViolationRights::CHEATING_MODIFIED_CLIENT},
    {"CHEATING_HACKING", RuleViolationRights::CHEATING_HACKING},
    {"CHEATING_MULTI_CLIENT", RuleViolationRights::CHEATING_MULTI_CLIENT},
    {"CHEATING_ACCOUNT_TRADING", RuleViolationRights::CHEATING_ACCOUNT_TRADING},
    {"CHEATING_ACCOUNT_SHARING", RuleViolationRights::CHEATING_ACCOUNT_SHARING},
    {"GAMEMASTER_THREATENING", RuleViolationRights::GAMEMASTER_THREATENING},
    {"GAMEMASTER_PRETENDING", RuleViolationRights::GAMEMASTER_PRETENDING},
    {"GAMEMASTER_INFLUENCE", RuleViolationRights::GAMEMASTER_INFLUENCE},
    {"GAMEMASTER_FALSE_REPORTS", RuleViolationRights::GAMEMASTER_FALSE_REPORTS},
    {"KILLING_EXCESSIVE_UNJUSTIFIED", RuleViolationRights::KILLING_EXCESSIVE_UNJUSTIFIED},
    {"DESTRUCTIVE_BEHAVIOUR", RuleViolationRights::DESTRUCTIVE_BEHAVIOUR},
    {"SPOILING_AUCTION", RuleViolationRights::SPOILING_AUCTION},
    {"INVALID_PAYMENT", RuleViolationRights::INVALID_PAYMENT}};

bool Groups::load()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/XML/groups.xml");
	if (!result) {
		printXMLError("Error - Groups::load", "data/XML/groups.xml", result);
		return false;
	}

	for (auto groupNode : doc.child("groups").children()) {
		Group group;
		group.id = pugi::cast<uint32_t>(groupNode.attribute("id").value());
		group.name = groupNode.attribute("name").as_string();
		group.access = groupNode.attribute("access").as_bool();
		group.maxDepotItems = pugi::cast<uint32_t>(groupNode.attribute("maxdepotitems").value());
		group.maxVipEntries = pugi::cast<uint32_t>(groupNode.attribute("maxvipentries").value());
		group.flags = pugi::cast<uint64_t>(groupNode.attribute("flags").value());
		if (pugi::xml_node node = groupNode.child("flags")) {
			for (auto flagNode : node.children()) {
				pugi::xml_attribute attr = flagNode.first_attribute();
				if (!attr || !attr.as_bool()) {
					continue;
				}

				auto parseFlag = ParsePlayerFlagMap.find(attr.name());
				if (parseFlag != ParsePlayerFlagMap.end()) {
					group.flags |= parseFlag->second;
				}
			}
		}

		if (pugi::xml_node node = groupNode.child("ruleviolations")) {
			for (auto flagNode : node.children()) {
				pugi::xml_attribute attr = flagNode.attribute("name");
				if (!attr) {
					continue;
				}

				auto parseFlag = ParseRuleViolationRightsMap.find(attr.as_string());
				if (parseFlag != ParseRuleViolationRightsMap.end()) {
					group.ruleViolationRights.insert(parseFlag->second);
				}
			}
		}

		groups.push_back(group);
	}
	return true;
}

Group* Groups::getGroup(uint16_t id)
{
	for (Group& group : groups) {
		if (group.id == id) {
			return &group;
		}
	}
	return nullptr;
}
