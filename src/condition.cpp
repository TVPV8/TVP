// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "condition.h"
#include "game.h"
#include "scriptwriter.h"

extern Game g_game;

bool Condition::setParam(ConditionParam_t param, int32_t value)
{
	switch (param) {
		case CONDITION_PARAM_TICKS: {
			ticks = value;
			return true;
		}

		case CONDITION_PARAM_BUFF_SPELL: {
			isBuff = (value != 0);
			return true;
		}

		case CONDITION_PARAM_SUBID: {
			subId = value;
			return true;
		}

		case CONDITION_PARAM_AGGRESSIVE: {
			aggressive = (value != 0);
			return true;
		}

		case CONDITION_PARAM_CYCLE: {
			cycle = value;
			return true;
		}

		case CONDITION_PARAM_MINCYCLE: {
			minCycle = value;
			return true;
		}

		case CONDITION_PARAM_COUNT: {
			count = value;
			return true;
		}

		case CONDITION_PARAM_MAX_COUNT: {
			maxCount = value;
			return true;
		}

		case CONDITION_PARAM_FACTORPERCENT: {
			factorPercent = value;
			return true;
		}

		default: {
			return false;
		}
	}
}

int32_t Condition::getParam(ConditionParam_t param)
{
	switch (param) {
		case CONDITION_PARAM_TICKS:
			return ticks;

		case CONDITION_PARAM_BUFF_SPELL:
			return isBuff ? 1 : 0;

		case CONDITION_PARAM_SUBID:
			return subId;

		case CONDITION_PARAM_CYCLE:
			return cycle;

		case CONDITION_PARAM_MINCYCLE:
			return minCycle;

		case CONDITION_PARAM_COUNT:
			return count;

		case CONDITION_PARAM_MAX_COUNT:
			return maxCount;

		case CONDITION_PARAM_FACTORPERCENT:
			return factorPercent;

		default:
			return std::numeric_limits<int32_t>().max();
	}
}

bool Condition::unserializeTVPFormat(ScriptReader& script)
{
	id = static_cast<ConditionId_t>(script.readNumber());
	script.readSymbol(',');
	ticks = script.readNumber();
	script.readSymbol(',');
	isBuff = script.readNumber();
	script.readSymbol(',');
	subId = script.readNumber();
	script.readSymbol(',');
	aggressive = script.readNumber();
	script.readSymbol(',');
	cycle = script.readNumber();
	script.readSymbol(',');
	count = script.readNumber();
	script.readSymbol(',');
	maxCount = script.readNumber();
	script.readSymbol(',');
	factorPercent = script.readNumber();
	return true;
}

void Condition::serializeTVPFormat(ScriptWriter& script)
{
	script.writeText("Condition = (");
	script.writeNumber(conditionType);
	script.writeText(",");
	script.writeNumber(id);
	script.writeText(",");
	script.writeNumber(ticks);
	script.writeText(",");
	script.writeNumber(isBuff);
	script.writeText(",");
	script.writeNumber(subId);
	script.writeText(",");
	script.writeNumber(aggressive);
	script.writeText(",");
	script.writeNumber(cycle);
	script.writeText(",");
	script.writeNumber(count);
	script.writeText(",");
	script.writeNumber(maxCount);
	script.writeText(",");
	script.writeNumber(factorPercent);
}

void Condition::setTicks(int32_t newTicks)
{
	ticks = newTicks;
	endTime = ticks + OTSYS_TIME();
}

bool Condition::executeCondition(Creature*, int32_t interval)
{
	if (ticks == -1) {
		return true;
	}

	// Not using set ticks here since it would reset endTime
	ticks = std::max<int32_t>(0, ticks - interval);
	return getEndTime() >= OTSYS_TIME();
}

Condition* Condition::createCondition(ConditionId_t id, ConditionType_t type, int32_t ticks, int32_t param /* = 0*/,
                                      bool buff /* = false*/, uint32_t subId /* = 0*/, bool aggressive /* = false */)
{
	switch (type) {
		case CONDITION_POISON:
		case CONDITION_FIRE:
		case CONDITION_ENERGY:
		case CONDITION_BLEEDING:
			return new ConditionDamage(id, type, buff, subId, aggressive);

		case CONDITION_HASTE:
		case CONDITION_PARALYZE:
			return new ConditionSpeed(id, type, ticks, buff, subId, param, aggressive);

		case CONDITION_INVISIBLE:
			return new ConditionInvisible(id, type, ticks, buff, subId, aggressive);

		case CONDITION_OUTFIT:
			return new ConditionOutfit(id, type, ticks, buff, subId, aggressive);

		case CONDITION_LIGHT:
			return new ConditionLight(id, type, ticks, buff, subId, param & 0xFF, (param & 0xFF00) >> 8, aggressive);

		case CONDITION_REGENERATION:
			return new ConditionRegeneration(id, type, ticks, buff, subId, aggressive);

		case CONDITION_SOUL:
			return new ConditionSoul(id, type, ticks, buff, subId, aggressive);

		case CONDITION_ATTRIBUTES:
			return new ConditionAttributes(id, type, ticks, buff, subId, aggressive);

		case CONDITION_DRUNK:
			return new ConditionDrunk(id, type, ticks, buff, subId, aggressive);

		case CONDITION_INFIGHT:
		case CONDITION_MUTED:
		case CONDITION_CHANNELMUTEDTICKS:
		case CONDITION_YELLTICKS:
		case CONDITION_PACIFIED:
		case CONDITION_MANASHIELD:
			return new ConditionGeneric(id, type, ticks, buff, subId, aggressive);

		default:
			return nullptr;
	}
}

bool Condition::startCondition(Creature*)
{
	if (ticks > 0) {
		endTime = ticks + OTSYS_TIME();
	}
	return true;
}

bool Condition::updateCondition(const Condition* addCondition)
{
	if (conditionType != addCondition->getType()) {
		return false;
	}

	if (ticks == -1 && addCondition->getTicks() > 0) {
		return false;
	}

	if (addCondition->cycle >= cycle) {
		return true;
	}

	if (addCondition->maxCount && addCondition->cycle < cycle) {
		return false;
	}

	if (addCondition->getTicks() >= 0 && getEndTime() > (OTSYS_TIME() + addCondition->getTicks())) {
		return false;
	}

	return true;
}

bool ConditionGeneric::startCondition(Creature* creature) { return Condition::startCondition(creature); }

bool ConditionGeneric::executeCondition(Creature* creature, int32_t interval)
{
	return Condition::executeCondition(creature, interval);
}

void ConditionGeneric::endCondition(Creature*)
{
	//
}

void ConditionGeneric::addCondition(Creature*, const Condition* condition)
{
	if (updateCondition(condition)) {
		setTicks(condition->getTicks());
	}
}

uint32_t ConditionGeneric::getIcons() const
{
	uint32_t icons = Condition::getIcons();

	switch (conditionType) {
		case CONDITION_MANASHIELD:
			icons |= ICON_MANASHIELD;
			break;

		case CONDITION_INFIGHT:
			icons |= ICON_SWORDS;
			break;

		default:
			break;
	}

	return icons;
}

void ConditionAttributes::addCondition(Creature* creature, const Condition* condition)
{
	if (updateCondition(condition)) {
		setTicks(condition->getTicks());

		const ConditionAttributes& conditionAttrs = static_cast<const ConditionAttributes&>(*condition);
		// Remove the old condition
		endCondition(creature);

		// Apply the new one
		memcpy(skills, conditionAttrs.skills, sizeof(skills));
		memcpy(specialSkills, conditionAttrs.specialSkills, sizeof(specialSkills));
		memcpy(skillsPercent, conditionAttrs.skillsPercent, sizeof(skillsPercent));
		memcpy(stats, conditionAttrs.stats, sizeof(stats));
		memcpy(statsPercent, conditionAttrs.statsPercent, sizeof(statsPercent));
		disableDefense = conditionAttrs.disableDefense;

		if (Player* player = creature->getPlayer()) {
			updatePercentSkills(player);
			updateSkills(player);
			updatePercentStats(player);
			updateStats(player);
		}
	}
}

void ConditionAttributes::serializeTVPFormat(ScriptWriter& script)
{
	Condition::serializeTVPFormat(script);

	script.writeText(",");

	script.writeNumber(skills[SKILL_FIRST]);
	script.writeText(",");
	script.writeNumber(skills[SKILL_CLUB]);
	script.writeText(",");
	script.writeNumber(skills[SKILL_SWORD]);
	script.writeText(",");
	script.writeNumber(skills[SKILL_AXE]);
	script.writeText(",");
	script.writeNumber(skills[SKILL_DISTANCE]);
	script.writeText(",");
	script.writeNumber(skills[SKILL_SHIELD]);
	script.writeText(",");
	script.writeNumber(skills[SKILL_LAST]);
	script.writeText(",");

	script.writeNumber(stats[STAT_FIRST]);
	script.writeText(",");
	script.writeNumber(stats[STAT_MAXMANAPOINTS]);
	script.writeText(",");
	script.writeNumber(stats[STAT_SOULPOINTS]);
	script.writeText(",");
	script.writeNumber(stats[STAT_LAST]);

	script.writeText(",");
	script.writeNumber(disableDefense);
	script.writeText(",");

	script.writeNumber(specialSkills[SPECIALSKILL_CRITICALHITCHANCE]);
	script.writeText(",");
	script.writeNumber(specialSkills[SPECIALSKILL_CRITICALHITAMOUNT]);
	script.writeText(",");
	script.writeNumber(specialSkills[SPECIALSKILL_LIFELEECHCHANCE]);
	script.writeText(",");
	script.writeNumber(specialSkills[SPECIALSKILL_LIFELEECHAMOUNT]);
	script.writeText(",");
	script.writeNumber(specialSkills[SPECIALSKILL_MANALEECHCHANCE]);
	script.writeText(",");
	script.writeNumber(specialSkills[SPECIALSKILL_MANALEECHAMOUNT]);
	script.writeText(",");
	script.writeNumber(specialSkills[SPECIALSKILL_LAST]);
}

bool ConditionAttributes::unserializeTVPFormat(ScriptReader& script)
{
	if (!Condition::unserializeTVPFormat(script)) {
		return false;
	}

	script.readSymbol(',');
	skills[SKILL_FIRST] = script.readNumber();
	script.readSymbol(',');
	skills[SKILL_CLUB] = script.readNumber();
	script.readSymbol(',');
	skills[SKILL_SWORD] = script.readNumber();
	script.readSymbol(',');
	skills[SKILL_AXE] = script.readNumber();
	script.readSymbol(',');
	skills[SKILL_DISTANCE] = script.readNumber();
	script.readSymbol(',');
	skills[SKILL_SHIELD] = script.readNumber();
	script.readSymbol(',');
	skills[SKILL_LAST] = script.readNumber();
	script.readSymbol(',');

	stats[STAT_FIRST] = script.readNumber();
	script.readSymbol(',');
	stats[STAT_MAXMANAPOINTS] = script.readNumber();
	script.readSymbol(',');
	stats[STAT_SOULPOINTS] = script.readNumber();
	script.readSymbol(',');
	stats[STAT_LAST] = script.readNumber();

	script.readSymbol(',');
	disableDefense = script.readNumber();
	script.readSymbol(',');

	specialSkills[SPECIALSKILL_CRITICALHITCHANCE] = script.readNumber();
	script.readSymbol(',');
	specialSkills[SPECIALSKILL_CRITICALHITAMOUNT] = script.readNumber();
	script.readSymbol(',');
	specialSkills[SPECIALSKILL_LIFELEECHCHANCE] = script.readNumber();
	script.readSymbol(',');
	specialSkills[SPECIALSKILL_LIFELEECHAMOUNT] = script.readNumber();
	script.readSymbol(',');
	specialSkills[SPECIALSKILL_MANALEECHCHANCE] = script.readNumber();
	script.readSymbol(',');
	specialSkills[SPECIALSKILL_MANALEECHAMOUNT] = script.readNumber();
	script.readSymbol(',');
	specialSkills[SPECIALSKILL_LAST] = script.readNumber();
	return true;
}

bool ConditionAttributes::startCondition(Creature* creature)
{
	if (!Condition::startCondition(creature)) {
		return false;
	}

	creature->setUseDefense(!disableDefense);

	if (Player* player = creature->getPlayer()) {
		updatePercentSkills(player);
		updateSkills(player);
		updatePercentStats(player);
		updateStats(player);
	}

	return true;
}

void ConditionAttributes::updatePercentStats(Player* player)
{
	for (int32_t i = STAT_FIRST; i <= STAT_LAST; ++i) {
		if (statsPercent[i] == 0) {
			continue;
		}

		switch (i) {
			case STAT_MAXHITPOINTS:
				stats[i] = static_cast<int32_t>(player->getMaxHealth() * ((statsPercent[i] - 100) / 100.f));
				break;

			case STAT_MAXMANAPOINTS:
				stats[i] = static_cast<int32_t>(player->getMaxMana() * ((statsPercent[i] - 100) / 100.f));
				break;

			case STAT_MAGICPOINTS:
				stats[i] = static_cast<int32_t>(player->getBaseMagicLevel() * ((statsPercent[i] - 100) / 100.f));
				break;
		}
	}
}

void ConditionAttributes::updateStats(Player* player)
{
	bool needUpdateStats = false;

	for (int32_t i = STAT_FIRST; i <= STAT_LAST; ++i) {
		if (stats[i]) {
			needUpdateStats = true;
			player->setVarStats(static_cast<stats_t>(i), stats[i]);
		}
	}

	if (needUpdateStats) {
		player->sendStats();
	}
}

void ConditionAttributes::updatePercentSkills(Player* player)
{
	for (uint8_t i = SKILL_FIRST; i <= SKILL_LAST; ++i) {
		if (skillsPercent[i] == 0) {
			continue;
		}

		int32_t unmodifiedSkill = player->getBaseSkill(i);
		skills[i] = static_cast<int32_t>(unmodifiedSkill * ((skillsPercent[i] - 100) / 100.f));
	}
}

void ConditionAttributes::updateSkills(Player* player)
{
	bool needUpdateSkills = false;

	for (int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i) {
		if (skills[i]) {
			needUpdateSkills = true;
			player->setVarSkill(static_cast<skills_t>(i), skills[i]);
		}
	}

	for (int32_t i = SPECIALSKILL_FIRST; i <= SPECIALSKILL_LAST; ++i) {
		if (specialSkills[i]) {
			needUpdateSkills = true;
			player->setVarSpecialSkill(static_cast<SpecialSkills_t>(i), specialSkills[i]);
		}
	}

	if (needUpdateSkills) {
		player->sendSkills();
	}
}

bool ConditionAttributes::executeCondition(Creature* creature, int32_t interval)
{
	return ConditionGeneric::executeCondition(creature, interval);
}

void ConditionAttributes::endCondition(Creature* creature)
{
	Player* player = creature->getPlayer();
	if (player) {
		bool needUpdateSkills = false;

		for (int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i) {
			if (skills[i] || skillsPercent[i]) {
				needUpdateSkills = true;
				player->setVarSkill(static_cast<skills_t>(i), -skills[i]);
			}
		}

		for (int32_t i = SPECIALSKILL_FIRST; i <= SPECIALSKILL_LAST; ++i) {
			if (specialSkills[i]) {
				needUpdateSkills = true;
				player->setVarSpecialSkill(static_cast<SpecialSkills_t>(i), -specialSkills[i]);
			}
		}

		if (needUpdateSkills) {
			player->sendSkills();
		}

		bool needUpdateStats = false;

		for (int32_t i = STAT_FIRST; i <= STAT_LAST; ++i) {
			if (stats[i]) {
				needUpdateStats = true;
				player->setVarStats(static_cast<stats_t>(i), -stats[i]);
			}
		}

		if (needUpdateStats) {
			player->sendStats();
		}
	}

	if (disableDefense) {
		creature->setUseDefense(true);
	}
}

bool ConditionAttributes::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = ConditionGeneric::setParam(param, value);

	switch (param) {
		case CONDITION_PARAM_SKILL_MELEE: {
			skills[SKILL_CLUB] = value;
			skills[SKILL_AXE] = value;
			skills[SKILL_SWORD] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_MELEEPERCENT: {
			skillsPercent[SKILL_CLUB] = value;
			skillsPercent[SKILL_AXE] = value;
			skillsPercent[SKILL_SWORD] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_FIST: {
			skills[SKILL_FIST] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_FISTPERCENT: {
			skillsPercent[SKILL_FIST] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_CLUB: {
			skills[SKILL_CLUB] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_CLUBPERCENT: {
			skillsPercent[SKILL_CLUB] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_SWORD: {
			skills[SKILL_SWORD] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_SWORDPERCENT: {
			skillsPercent[SKILL_SWORD] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_AXE: {
			skills[SKILL_AXE] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_AXEPERCENT: {
			skillsPercent[SKILL_AXE] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_DISTANCE: {
			skills[SKILL_DISTANCE] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_DISTANCEPERCENT: {
			skillsPercent[SKILL_DISTANCE] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_SHIELD: {
			skills[SKILL_SHIELD] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_SHIELDPERCENT: {
			skillsPercent[SKILL_SHIELD] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_FISHING: {
			skills[SKILL_FISHING] = value;
			return true;
		}

		case CONDITION_PARAM_SKILL_FISHINGPERCENT: {
			skillsPercent[SKILL_FISHING] = value;
			return true;
		}

		case CONDITION_PARAM_STAT_MAXHITPOINTS: {
			stats[STAT_MAXHITPOINTS] = value;
			return true;
		}

		case CONDITION_PARAM_STAT_MAXMANAPOINTS: {
			stats[STAT_MAXMANAPOINTS] = value;
			return true;
		}

		case CONDITION_PARAM_STAT_MAGICPOINTS: {
			stats[STAT_MAGICPOINTS] = value;
			return true;
		}

		case CONDITION_PARAM_STAT_MAXHITPOINTSPERCENT: {
			statsPercent[STAT_MAXHITPOINTS] = std::max<int32_t>(0, value);
			return true;
		}

		case CONDITION_PARAM_STAT_MAXMANAPOINTSPERCENT: {
			statsPercent[STAT_MAXMANAPOINTS] = std::max<int32_t>(0, value);
			return true;
		}

		case CONDITION_PARAM_STAT_MAGICPOINTSPERCENT: {
			statsPercent[STAT_MAGICPOINTS] = std::max<int32_t>(0, value);
			return true;
		}

		case CONDITION_PARAM_DISABLE_DEFENSE: {
			disableDefense = (value != 0);
			return true;
		}

		case CONDITION_PARAM_SPECIALSKILL_CRITICALHITCHANCE: {
			specialSkills[SPECIALSKILL_CRITICALHITCHANCE] = value;
			return true;
		}

		case CONDITION_PARAM_SPECIALSKILL_CRITICALHITAMOUNT: {
			specialSkills[SPECIALSKILL_CRITICALHITAMOUNT] = value;
			return true;
		}

		case CONDITION_PARAM_SPECIALSKILL_LIFELEECHCHANCE: {
			specialSkills[SPECIALSKILL_LIFELEECHCHANCE] = value;
			return true;
		}

		case CONDITION_PARAM_SPECIALSKILL_LIFELEECHAMOUNT: {
			specialSkills[SPECIALSKILL_LIFELEECHAMOUNT] = value;
			return true;
		}

		case CONDITION_PARAM_SPECIALSKILL_MANALEECHCHANCE: {
			specialSkills[SPECIALSKILL_MANALEECHCHANCE] = value;
			return true;
		}

		case CONDITION_PARAM_SPECIALSKILL_MANALEECHAMOUNT: {
			specialSkills[SPECIALSKILL_MANALEECHAMOUNT] = value;
			return true;
		}

		case CONDITION_PARAM_AGGRESSIVE: {
			aggressive = (value != 0);
			return true;
		}

		default:
			return ret;
	}
}

int32_t ConditionAttributes::getParam(ConditionParam_t param)
{
	switch (param) {
		case CONDITION_PARAM_SKILL_FIST:
			return skills[SKILL_FIST];

		case CONDITION_PARAM_SKILL_FISTPERCENT:
			return skillsPercent[SKILL_FIST];

		case CONDITION_PARAM_SKILL_CLUB:
			return skills[SKILL_CLUB];

		case CONDITION_PARAM_SKILL_CLUBPERCENT:
			return skillsPercent[SKILL_CLUB];

		case CONDITION_PARAM_SKILL_SWORD:
			return skills[SKILL_SWORD];

		case CONDITION_PARAM_SKILL_SWORDPERCENT:
			return skillsPercent[SKILL_SWORD];

		case CONDITION_PARAM_SKILL_AXE:
			return skills[SKILL_AXE];

		case CONDITION_PARAM_SKILL_AXEPERCENT:
			return skillsPercent[SKILL_AXE];

		case CONDITION_PARAM_SKILL_DISTANCE:
			return skills[SKILL_DISTANCE];

		case CONDITION_PARAM_SKILL_DISTANCEPERCENT:
			return skillsPercent[SKILL_DISTANCE];

		case CONDITION_PARAM_SKILL_SHIELD:
			return skills[SKILL_SHIELD];

		case CONDITION_PARAM_SKILL_SHIELDPERCENT:
			return skillsPercent[SKILL_SHIELD];

		case CONDITION_PARAM_SKILL_FISHING:
			return skills[SKILL_FISHING];

		case CONDITION_PARAM_SKILL_FISHINGPERCENT:
			return skillsPercent[SKILL_FISHING];

		case CONDITION_PARAM_STAT_MAXHITPOINTS:
			return stats[STAT_MAXHITPOINTS];

		case CONDITION_PARAM_STAT_MAXMANAPOINTS:
			return stats[STAT_MAXMANAPOINTS];

		case CONDITION_PARAM_STAT_MAGICPOINTS:
			return stats[STAT_MAGICPOINTS];

		case CONDITION_PARAM_STAT_MAXHITPOINTSPERCENT:
			return statsPercent[STAT_MAXHITPOINTS];

		case CONDITION_PARAM_STAT_MAXMANAPOINTSPERCENT:
			return statsPercent[STAT_MAXMANAPOINTS];

		case CONDITION_PARAM_STAT_MAGICPOINTSPERCENT:
			return statsPercent[STAT_MAGICPOINTS];

		case CONDITION_PARAM_DISABLE_DEFENSE:
			return disableDefense ? 1 : 0;

		case CONDITION_PARAM_SPECIALSKILL_CRITICALHITCHANCE:
			return specialSkills[SPECIALSKILL_CRITICALHITCHANCE];

		case CONDITION_PARAM_SPECIALSKILL_CRITICALHITAMOUNT:
			return specialSkills[SPECIALSKILL_CRITICALHITAMOUNT];

		case CONDITION_PARAM_SPECIALSKILL_LIFELEECHCHANCE:
			return specialSkills[SPECIALSKILL_LIFELEECHCHANCE];

		case CONDITION_PARAM_SPECIALSKILL_LIFELEECHAMOUNT:
			return specialSkills[SPECIALSKILL_LIFELEECHAMOUNT];

		case CONDITION_PARAM_SPECIALSKILL_MANALEECHCHANCE:
			return specialSkills[SPECIALSKILL_MANALEECHCHANCE];

		case CONDITION_PARAM_SPECIALSKILL_MANALEECHAMOUNT:
			return specialSkills[SPECIALSKILL_MANALEECHAMOUNT];

		default:
			return ConditionGeneric::getParam(param);
	}
}

void ConditionRegeneration::addCondition(Creature*, const Condition* condition)
{
	if (updateCondition(condition)) {
		if (condition->getTicks() != -1) setTicks(condition->getTicks());

		const ConditionRegeneration& conditionRegen = static_cast<const ConditionRegeneration&>(*condition);

		healthTicks = conditionRegen.healthTicks;
		manaTicks = conditionRegen.manaTicks;

		healthGain = conditionRegen.healthGain;
		manaGain = conditionRegen.manaGain;
	}
}

void ConditionRegeneration::serializeTVPFormat(ScriptWriter& script)
{
	Condition::serializeTVPFormat(script);

	script.writeText(",");
	script.writeNumber(healthTicks);
	script.writeText(",");
	script.writeNumber(healthGain);
	script.writeText(",");
	script.writeNumber(manaTicks);
	script.writeText(",");
	script.writeNumber(manaGain);
}

bool ConditionRegeneration::unserializeTVPFormat(ScriptReader& script)
{
	if (!Condition::unserializeTVPFormat(script)) {
		return false;
	}

	script.readSymbol(',');
	healthTicks = script.readNumber();
	script.readSymbol(',');
	healthGain = script.readNumber();
	script.readSymbol(',');
	manaTicks = script.readNumber();
	script.readSymbol(',');
	manaGain = script.readNumber();
	return true;
}

bool ConditionRegeneration::executeCondition(Creature* creature, int32_t interval)
{
	internalHealthTicks += interval;
	internalManaTicks += interval;

	if (creature->getZone() == ZONE_PROTECTION) {
		return ConditionGeneric::executeCondition(creature, interval);
	}

	if (internalHealthTicks >= healthTicks) {
		internalHealthTicks = 0;
		creature->changeHealth(healthGain);
	}

	if (internalManaTicks >= manaTicks) {
		internalManaTicks = 0;

		if (Player* player = creature->getPlayer()) {
			player->changeMana(manaGain);
		}
	}

	return ConditionGeneric::executeCondition(creature, interval);
}

bool ConditionRegeneration::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = ConditionGeneric::setParam(param, value);

	switch (param) {
		case CONDITION_PARAM_HEALTHGAIN:
			healthGain = value;
			return true;

		case CONDITION_PARAM_HEALTHTICKS:
			healthTicks = value;
			return true;

		case CONDITION_PARAM_MANAGAIN:
			manaGain = value;
			return true;

		case CONDITION_PARAM_MANATICKS:
			manaTicks = value;
			return true;

		default:
			return ret;
	}
}

int32_t ConditionRegeneration::getParam(ConditionParam_t param)
{
	switch (param) {
		case CONDITION_PARAM_HEALTHGAIN:
			return healthGain;

		case CONDITION_PARAM_HEALTHTICKS:
			return healthTicks;

		case CONDITION_PARAM_MANAGAIN:
			return manaGain;

		case CONDITION_PARAM_MANATICKS:
			return manaTicks;

		default:
			return ConditionGeneric::getParam(param);
	}
}

void ConditionSoul::addCondition(Creature*, const Condition* condition)
{
	if (updateCondition(condition)) {
		setTicks(condition->getTicks());

		const ConditionSoul& conditionSoul = static_cast<const ConditionSoul&>(*condition);

		soulTicks = conditionSoul.soulTicks;
		soulGain = conditionSoul.soulGain;
	}
}

void ConditionSoul::serializeTVPFormat(ScriptWriter& script)
{
	Condition::serializeTVPFormat(script);

	script.writeText(",");
	script.writeNumber(soulGain);
	script.writeText(",");
	script.writeNumber(soulTicks);
}

bool ConditionSoul::unserializeTVPFormat(ScriptReader& script)
{
	if (!Condition::unserializeTVPFormat(script)) {
		return false;
	}

	script.readSymbol(',');
	soulGain = script.readNumber();
	script.readSymbol(',');
	soulTicks = script.readNumber();
	return true;
}

bool ConditionSoul::executeCondition(Creature* creature, int32_t interval)
{
	internalSoulTicks += interval;

	if (Player* player = creature->getPlayer()) {
		if (player->getZone() != ZONE_PROTECTION) {
			if (internalSoulTicks >= soulTicks) {
				internalSoulTicks = 0;
				player->changeSoul(soulGain);
			}
		}
	}

	return ConditionGeneric::executeCondition(creature, interval);
}

bool ConditionSoul::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = ConditionGeneric::setParam(param, value);
	switch (param) {
		case CONDITION_PARAM_SOULGAIN:
			soulGain = value;
			return true;

		case CONDITION_PARAM_SOULTICKS:
			soulTicks = value;
			return true;

		default:
			return ret;
	}
}

int32_t ConditionSoul::getParam(ConditionParam_t param)
{
	switch (param) {
		case CONDITION_PARAM_SOULGAIN:
			return soulGain;

		case CONDITION_PARAM_SOULTICKS:
			return soulTicks;

		default:
			return ConditionGeneric::getParam(param);
	}
}

bool ConditionDamage::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = Condition::setParam(param, value);

	switch (param) {
		case CONDITION_PARAM_OWNER:
			owner = value;
			return true;

		case CONDITION_PARAM_OWNERGUID:
			ownerGuid = value;
			return true;

		case CONDITION_PARAM_FORCEUPDATE:
			forceUpdate = (value != 0);
			return true;

		case CONDITION_PARAM_DELAYED:
			delayed = (value != 0);
			return true;

		case CONDITION_PARAM_MAXVALUE:
			maxDamage = std::abs(value);
			break;

		case CONDITION_PARAM_MINVALUE:
			minDamage = std::abs(value);
			break;

		case CONDITION_PARAM_STARTVALUE:
			startDamage = std::abs(value);
			break;

		case CONDITION_PARAM_TICKINTERVAL:
			tickInterval = std::abs(value);
			break;

		case CONDITION_PARAM_PERIODICDAMAGE:
			periodDamage = value;
			break;

		case CONDITION_PARAM_FIELD:
			field = (value != 0);
			break;

		default:
			return false;
	}

	return ret;
}

int32_t ConditionDamage::getParam(ConditionParam_t param)
{
	switch (param) {
		case CONDITION_PARAM_OWNER:
			return owner;

		case CONDITION_PARAM_OWNERGUID:
			return ownerGuid;

		case CONDITION_PARAM_FORCEUPDATE:
			return forceUpdate ? 1 : 0;

		case CONDITION_PARAM_DELAYED:
			return delayed ? 1 : 0;

		case CONDITION_PARAM_MAXVALUE:
			return maxDamage;

		case CONDITION_PARAM_MINVALUE:
			return minDamage;

		case CONDITION_PARAM_STARTVALUE:
			return startDamage;

		case CONDITION_PARAM_TICKINTERVAL:
			return tickInterval;

		case CONDITION_PARAM_PERIODICDAMAGE:
			return periodDamage;

		case CONDITION_PARAM_FIELD:
			return field ? 1 : 0;

		default:
			return Condition::getParam(param);
	}
}

void ConditionDamage::serializeTVPFormat(ScriptWriter& script)
{
	Condition::serializeTVPFormat(script);

	script.writeText(",");
	script.writeNumber(delayed);
	script.writeText(",");
	script.writeNumber(periodDamage);
	script.writeText(",");
	script.writeNumber(damageList.size());
	if (damageList.size() > 0) {
		script.writeText(",");
		for (int32_t i = 0; i <= damageList.size() - 1; i++) {
			auto it = damageList.begin();
			std::advance(it, i);
			const IntervalInfo& internalInfo = *it;

			script.writeNumber(internalInfo.interval);
			script.writeText(",");
			script.writeNumber(internalInfo.timeLeft);
			script.writeText(",");
			script.writeNumber(internalInfo.value);

			if (i < damageList.size() - 1) {
				script.writeText(",");
			}
		}
	}
}

bool ConditionDamage::unserializeTVPFormat(ScriptReader& script)
{
	if (!Condition::unserializeTVPFormat(script)) {
		return false;
	}

	script.readSymbol(',');
	delayed = script.readNumber();
	script.readSymbol(',');
	periodDamage = script.readNumber();
	script.readSymbol(',');
	const int32_t totalDamageList = script.readNumber();
	if (totalDamageList > 0) {
		script.readSymbol(',');
		for (int32_t i = 0; i <= totalDamageList - 1; i++) {
			IntervalInfo info;
			info.interval = script.readNumber();
			script.readSymbol(',');
			info.timeLeft = script.readNumber();
			script.readSymbol(',');
			info.value = script.readNumber();
			if (i < totalDamageList - 1) {
				script.readSymbol(',');
			}
		}
	}

	return true;
}

bool ConditionDamage::updateCondition(const Condition* addCondition)
{
	const ConditionDamage& conditionDamage = static_cast<const ConditionDamage&>(*addCondition);
	if (conditionDamage.doForceUpdate()) {
		return true;
	}

	if (cycle > 0) {
		return conditionDamage.cycle >= cycle;
	}

	if (count > 0) {
		if (ticks == -1 && conditionDamage.ticks > 0) {
			return false;
		}
	}

	return conditionDamage.getTotalDamage() >= getTotalDamage();
}

bool ConditionDamage::addDamage(int32_t rounds, int32_t time, int32_t value)
{
	time = std::max<int32_t>(time, EVENT_CREATURE_THINK_INTERVAL);
	if (rounds == -1) {
		// periodic damage
		periodDamage = value;
		setParam(CONDITION_PARAM_TICKINTERVAL, time);
		setParam(CONDITION_PARAM_TICKS, -1);
		return true;
	}

	if (periodDamage > 0) {
		return false;
	}

	// rounds, time, damage
	for (int32_t i = 0; i <= rounds; ++i) {
		IntervalInfo damageInfo{};
		damageInfo.interval = time;
		damageInfo.timeLeft = time;
		damageInfo.value = value;

		damageList.push_back(damageInfo);

		if (ticks != -1) {
			setTicks(ticks + damageInfo.interval);
		}
	}

	return true;
}

bool ConditionDamage::init()
{
	if (periodDamage != 0) {
		return true;
	}

	if (!damageList.empty()) {
		return true;
	}

	setTicks(0);

	int32_t amount = uniform_random(minDamage, maxDamage);
	if (amount != 0) {
		if (startDamage > maxDamage) {
			startDamage = maxDamage;
		} else if (startDamage == 0) {
			startDamage = std::max<int32_t>(1, std::ceil(amount / 20.0));
		}

		std::list<int32_t> list;
		ConditionDamage::generateDamageList(amount, startDamage, list);
		for (int32_t value : list) {
			addDamage(1, tickInterval, -value);
		}
	}
	return !damageList.empty();
}

bool ConditionDamage::startCondition(Creature* creature)
{
	if (!Condition::startCondition(creature)) {
		return false;
	}

	if (!delayed && initDamage != 0) {
		// delayed condition does no initial damage
		if (!doDamage(creature, initDamage)) {
			return false;
		}
	}

	if (cycle > 0) {
		setParam(CONDITION_PARAM_TICKINTERVAL, 1000);

		if (factorPercent == -1) {
			factorPercent = 50;
		}

		if (factorPercent <= 9) {
			factorPercent = 10;
		}

		if (factorPercent >= 1001) {
			factorPercent = 1000;
		}

		if (minCycle > 0) {
			cycle = random(minCycle, cycle);
		}
	} else {
		if (!init()) {
			return false;
		}
	}

	return true;
}

bool ConditionDamage::executeCondition(Creature* creature, int32_t interval)
{
	if (periodDamage != 0) {
		periodDamageTick += interval;

		if (periodDamageTick >= tickInterval) {
			periodDamageTick = 0;
			doDamage(creature, periodDamage);
		}
	} else if (!damageList.empty()) {
		IntervalInfo& damageInfo = damageList.front();

		bool bRemove = (ticks != -1);
		creature->onTickCondition(getType(), bRemove);
		damageInfo.timeLeft -= interval;

		if (damageInfo.timeLeft <= 0) {
			int32_t damage = damageInfo.value;

			if (bRemove) {
				damageList.pop_front();
			} else {
				damageInfo.timeLeft = damageInfo.interval;
			}

			doDamage(creature, damage);
		}

		if (!bRemove) {
			if (ticks > 0) {
				endTime += interval;
			}

			interval = 0;
		}
	} else if (cycle > 0) {
		const int32_t r_cycle = cycle;

		bool removeCondition = true;
		creature->onTickCondition(getType(), removeCondition);

		if (r_cycle) {
			if (count <= 0) {
				count = maxCount;

				if (getType() == CONDITION_FIRE) {
					if (removeCondition) {
						cycle = r_cycle + 2 * (r_cycle <= 0) - 1;
					}

					doDamage(creature, -10);
				} else if (getType() == CONDITION_ENERGY) {
					if (removeCondition) {
						cycle = r_cycle + 2 * (r_cycle <= 0) - 1;
					}

					doDamage(creature, -25);
				} else if (getType() == CONDITION_POISON) {
					int32_t f = factorPercent * r_cycle / 1000;
					if (!f) {
						f = 2 * (r_cycle > 0) - 1;
					}

					cycle = r_cycle - f;

					if (!removeCondition) {
						cycle++;
					}

					doDamage(creature, -f);
				}
			} else {
				--count;
			}
		} else {
			return false;
		}

		return cycle > 0;
	}

	return Condition::executeCondition(creature, interval);
}

void ConditionDamage::endCondition(Creature*)
{
	//
}

bool ConditionDamage::getNextDamage(int32_t& damage)
{
	if (periodDamage != 0) {
		damage = periodDamage;
		return true;
	} else if (!damageList.empty()) {
		IntervalInfo& damageInfo = damageList.front();
		damage = damageInfo.value;
		if (ticks != -1) {
			damageList.pop_front();
		}
		return true;
	}
	return false;
}

bool ConditionDamage::doDamage(Creature* creature, int32_t healthChange)
{
	if (creature->isSuppress(getType()) || creature->isImmune(getType())) {
		if (!creature->getPlayer()) {
			g_game.addMagicEffect(creature->getPosition(), CONST_ME_BLOCKHIT);
		}
		return false;
	}

	CombatDamage damage;
	damage.origin = ORIGIN_CONDITION;
	damage.value = healthChange;
	damage.type = Combat::ConditionToDamageType(conditionType);

	Creature* attacker = nullptr;
	if (ownerGuid) {
		attacker = g_game.getPlayerByGUID(ownerGuid);
	} else {
		attacker = g_game.getCreatureByID(owner);
	}

	if (!creature->isAttackable() || Combat::canDoCombat(attacker, creature) != RETURNVALUE_NOERROR) {
		if (!creature->isInGhostMode()) {
			g_game.addMagicEffect(creature->getPosition(), CONST_ME_POFF);
		}
		return false;
	}

	/*
	* Only 7.7 related
	if (attacker && attacker->getPlayer()) {
	    if (creature->getPlayer()) {
	        damage.value = (damage.value - 1) / 2;
	    }
	}
	*/

	if (g_game.combatBlockHit(damage, attacker, creature, false, false, field) != BLOCK_NONE) {
		return false;
	}

	return g_game.combatChangeHealth(attacker, creature, damage);
}

void ConditionDamage::addCondition(Creature* creature, const Condition* condition)
{
	if (condition->getType() != conditionType) {
		return;
	}

	const ConditionDamage& conditionDamage = static_cast<const ConditionDamage&>(*condition);

	// Apply step in damage careless if we override the condition or not
	if (cycle != 0) {
		if (conditionDamage.initDamage != 0) {
			doDamage(creature, conditionDamage.initDamage);
		}
	}

	if (!updateCondition(condition)) {
		return;
	}

	setTicks(condition->getTicks());
	owner = conditionDamage.owner;
	ownerGuid = conditionDamage.ownerGuid;
	maxDamage = conditionDamage.maxDamage;
	minDamage = conditionDamage.minDamage;
	startDamage = conditionDamage.startDamage;
	tickInterval = conditionDamage.tickInterval;
	periodDamage = conditionDamage.periodDamage;

	// Make sure we restart the counter for the next cycle
	if (cycle > 0 && conditionType != CONDITION_POISON) {
		count = conditionDamage.count;
	}

	maxCount = conditionDamage.maxCount;
	cycle = conditionDamage.cycle;

	if (!damageList.empty()) {
		damageList.clear();
	}

	damageList = conditionDamage.damageList;

	if (init()) {
		if (conditionDamage.initDamage != 0) {
			doDamage(creature, conditionDamage.initDamage);
		}
	}
}

int32_t ConditionDamage::getTotalDamage() const
{
	if (cycle > 0) {
		return cycle;
	}

	int32_t result;
	if (!damageList.empty()) {
		result = 0;
		for (const IntervalInfo& intervalInfo : damageList) {
			result += intervalInfo.value;
		}
	} else {
		result = minDamage + (maxDamage - minDamage) / 2;
	}
	return std::abs(result);
}

uint32_t ConditionDamage::getIcons() const
{
	uint32_t icons = Condition::getIcons();
	switch (conditionType) {
		case CONDITION_FIRE:
			icons |= ICON_BURN;
			break;

		case CONDITION_ENERGY:
			icons |= ICON_ENERGY;
			break;

		case CONDITION_POISON:
			icons |= ICON_POISON;
			break;
		default:
			break;
	}
	return icons;
}

void ConditionDamage::generateDamageList(int32_t amount, int32_t start, std::list<int32_t>& list)
{
	amount = std::abs(amount);
	int32_t sum = 0;
	double x1, x2;

	for (int32_t i = start; i > 0; --i) {
		int32_t n = start + 1 - i;
		int32_t med = (n * amount) / start;

		do {
			sum += i;
			list.push_back(i);

			x1 = std::fabs(1.0 - ((static_cast<float>(sum)) + i) / med);
			x2 = std::fabs(1.0 - (static_cast<float>(sum) / med));
		} while (x1 < x2);
	}
}

bool ConditionSpeed::setParam(ConditionParam_t param, int32_t value)
{
	Condition::setParam(param, value);

	switch (param) {
		case CONDITION_PARAM_SPEED:
			speedDelta = value;
			break;
		case CONDITION_PARAM_SPEEDVARIATION:
			speedVariation = value;
			break;
		default:
			return false;
	}

	if (speedDelta > 0) {
		conditionType = CONDITION_HASTE;
	} else {
		conditionType = CONDITION_PARALYZE;
	}
	return true;
}

int32_t ConditionSpeed::getParam(ConditionParam_t param)
{
	switch (param) {
		case CONDITION_PARAM_SPEED:
			return speedDelta;

		case CONDITION_PARAM_SPEEDVARIATION:
			return speedVariation;

		default:
			return Condition::getParam(param);
	}
}

void ConditionSpeed::serializeTVPFormat(ScriptWriter& script)
{
	Condition::serializeTVPFormat(script);

	script.writeText(",");
	script.writeNumber(storedSpeedDelta);
}

bool ConditionSpeed::unserializeTVPFormat(ScriptReader& script)
{
	if (!Condition::unserializeTVPFormat(script)) {
		return false;
	}

	script.readSymbol(',');
	storedSpeedDelta = script.readNumber();
	return true;
}

bool ConditionSpeed::startCondition(Creature* creature)
{
	if (!Condition::startCondition(creature)) {
		return false;
	}

	if (storedSpeedDelta != 0) {
		g_game.changeSpeed(creature, storedSpeedDelta);
		return true;
	}

	if (speedVariation != 0) {
		speedDelta = uniform_random(-speedVariation, speedVariation) + speedDelta;
	}

	int32_t formula;
	const int32_t baseSpeed = creature->getBaseSpeed();
	if (speedDelta >= -100) {
		formula = baseSpeed * speedDelta / 100;
	} else {
		formula = -20 - baseSpeed;
	}

	storedSpeedDelta = formula;
	g_game.changeSpeed(creature, formula);
	return true;
}

bool ConditionSpeed::executeCondition(Creature* creature, int32_t interval)
{
	return Condition::executeCondition(creature, interval);
}

void ConditionSpeed::endCondition(Creature* creature) { g_game.changeSpeed(creature, -storedSpeedDelta); }

void ConditionSpeed::addCondition(Creature* creature, const Condition* condition)
{
	if (conditionType != condition->getType()) {
		return;
	}

	if (ticks == -1 && condition->getTicks() > 0) {
		return;
	}

	setTicks(condition->getTicks());

	const ConditionSpeed& conditionSpeed = static_cast<const ConditionSpeed&>(*condition);
	g_game.changeSpeed(creature, -storedSpeedDelta); // remove previous var speed
	speedDelta = conditionSpeed.speedDelta;
	speedVariation = conditionSpeed.speedVariation;

	if (speedVariation != 0) {
		speedDelta = uniform_random(-speedVariation, speedVariation) + speedDelta;
	}

	int32_t formula;
	const int32_t baseSpeed = creature->getBaseSpeed();
	if (speedDelta >= -100) {
		formula = baseSpeed * speedDelta / 100;
	} else {
		formula = -20 - baseSpeed;
	}

	storedSpeedDelta = formula;
	g_game.changeSpeed(creature, formula);
}

uint32_t ConditionSpeed::getIcons() const
{
	uint32_t icons = Condition::getIcons();
	switch (conditionType) {
		case CONDITION_HASTE:
			icons |= ICON_HASTE;
			break;

		case CONDITION_PARALYZE:
			icons |= ICON_PARALYZE;
			break;

		default:
			break;
	}
	return icons;
}

bool ConditionInvisible::startCondition(Creature* creature)
{
	if (!Condition::startCondition(creature)) {
		return false;
	}

	if (!creature->isInGhostMode()) {
		g_game.internalCreatureChangeVisible(creature, false);
	}
	return true;
}

void ConditionInvisible::endCondition(Creature* creature)
{
	if (!creature->isInGhostMode() && !creature->isInvisible()) {
		g_game.internalCreatureChangeVisible(creature, true);
	}
}

void ConditionOutfit::setOutfit(const Outfit_t& outfit) { this->outfit = outfit; }

void ConditionOutfit::serializeTVPFormat(ScriptWriter& script)
{
	Condition::serializeTVPFormat(script);

	script.writeText(",");
	script.writeNumber(outfit.lookType);
	script.writeText(",");
	script.writeNumber(outfit.lookTypeEx);
	script.writeText(",");
	script.writeNumber(outfit.lookHead);
	script.writeText(",");
	script.writeNumber(outfit.lookBody);
	script.writeText(",");
	script.writeNumber(outfit.lookLegs);
	script.writeText(",");
	script.writeNumber(outfit.lookFeet);
}

bool ConditionOutfit::unserializeTVPFormat(ScriptReader& script)
{
	if (!Condition::unserializeTVPFormat(script)) {
		return false;
	}

	script.readSymbol(',');
	outfit.lookType = script.readNumber();
	script.readSymbol(',');
	outfit.lookTypeEx = script.readNumber();
	script.readSymbol(',');
	outfit.lookHead = script.readNumber();
	script.readSymbol(',');
	outfit.lookBody = script.readNumber();
	script.readSymbol(',');
	outfit.lookLegs = script.readNumber();
	script.readSymbol(',');
	outfit.lookFeet = script.readNumber();
	return true;
}

bool ConditionOutfit::startCondition(Creature* creature)
{
	if (!Condition::startCondition(creature)) {
		return false;
	}

	g_game.internalCreatureChangeOutfit(creature, outfit);
	return true;
}

bool ConditionOutfit::executeCondition(Creature* creature, int32_t interval)
{
	return Condition::executeCondition(creature, interval);
}

void ConditionOutfit::endCondition(Creature* creature)
{
	g_game.internalCreatureChangeOutfit(creature, creature->getDefaultOutfit());
}

void ConditionOutfit::addCondition(Creature* creature, const Condition* condition)
{
	if (updateCondition(condition)) {
		setTicks(condition->getTicks());

		const ConditionOutfit& conditionOutfit = static_cast<const ConditionOutfit&>(*condition);
		outfit = conditionOutfit.outfit;

		g_game.internalCreatureChangeOutfit(creature, outfit);
	}
}

bool ConditionLight::startCondition(Creature* creature)
{
	if (!Condition::startCondition(creature)) {
		return false;
	}

	if (cycle) {
		lightInfo.level = static_cast<uint8_t>(cycle);
	}

	if (lightInfo.level == 0) {
		return false;
	}

	internalLightTicks = 0;
	lightChangeInterval = ticks / lightInfo.level;
	return true;
}

bool ConditionLight::executeCondition(Creature* creature, int32_t interval)
{
	if (maxCount) {
		if (cycle) {
			if (count <= 0) {
				count = maxCount;
				cycle = 2 * (cycle <= 0) - 1 + cycle;
				lightInfo.level = cycle;

				g_game.changeLight(creature);
			} else {
				count--;
			}
		}

		return cycle > 0;
	}

	internalLightTicks += interval;

	if (internalLightTicks >= lightChangeInterval && ticks != -1) {
		internalLightTicks = 0;

		if (lightInfo.level > 0) {
			--lightInfo.level;
			g_game.changeLight(creature);
		}
	}

	return Condition::executeCondition(creature, interval);
}

void ConditionLight::endCondition(Creature* creature) { g_game.changeLight(creature); }

void ConditionLight::addCondition(Creature* creature, const Condition* condition)
{
	if (updateCondition(condition)) {
		setTicks(condition->getTicks());
		updateLightCycles(condition);

		const auto& conditionLight = dynamic_cast<const ConditionLight&>(*condition);
		if (conditionLight.lightInfo.level > 0) {
			lightInfo.level = conditionLight.lightInfo.level;
			lightInfo.color = conditionLight.lightInfo.color;
			lightChangeInterval = ticks / lightInfo.level;
		}
		internalLightTicks = 0;
		g_game.changeLight(creature);
	}
}

bool ConditionLight::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = Condition::setParam(param, value);
	if (ret) {
		return false;
	}

	switch (param) {
		case CONDITION_PARAM_LIGHT_LEVEL:
			lightInfo.level = value;
			return true;

		case CONDITION_PARAM_LIGHT_COLOR:
			lightInfo.color = value;
			return true;

		default:
			return false;
	}
}

int32_t ConditionLight::getParam(ConditionParam_t param)
{
	switch (param) {
		case CONDITION_PARAM_LIGHT_LEVEL:
			return lightInfo.level;

		case CONDITION_PARAM_LIGHT_COLOR:
			return lightInfo.color;

		default:
			return Condition::getParam(param);
	}
}

void ConditionLight::serializeTVPFormat(ScriptWriter& script)
{
	Condition::serializeTVPFormat(script);

	script.writeText(",");
	script.writeNumber(lightInfo.color);
	script.writeText(",");
	script.writeNumber(lightInfo.level);
	script.writeText(",");
	script.writeNumber(internalLightTicks);
	script.writeText(",");
	script.writeNumber(lightChangeInterval);
}

void ConditionLight::updateLightCycles(const Condition* condition)
{
	cycle = condition->getCycle();
	count = condition->getMaxCount();
	maxCount = condition->getMaxCount();
	minCycle = condition->getMinCycle();
	lightInfo.level = cycle;
}

bool ConditionLight::unserializeTVPFormat(ScriptReader& script)
{
	if (!Condition::unserializeTVPFormat(script)) {
		return false;
	}

	script.readSymbol(',');
	lightInfo.color = script.readNumber();
	script.readSymbol(',');
	lightInfo.level = script.readNumber();
	script.readSymbol(',');
	internalLightTicks = script.readNumber();
	script.readSymbol(',');
	lightChangeInterval = script.readNumber();
	return true;
}

bool ConditionDrunk::startCondition(Creature* creature)
{
	if (!Condition::startCondition(creature)) {
		return false;
	}

	return true;
}

bool ConditionDrunk::updateCondition(const Condition* addCondition)
{
	const ConditionDrunk* conditionDrunk = dynamic_cast<const ConditionDrunk*>(addCondition);
	return cycle <= conditionDrunk->getCycle() || conditionDrunk->getCycle() == 1;
}

void ConditionDrunk::addCondition(Creature*, const Condition* condition)
{
	if (!updateCondition(condition)) {
		return;
	}

	const ConditionDrunk* conditionDrunk = dynamic_cast<const ConditionDrunk*>(condition);

	setTicks(conditionDrunk->getTicks());
	if (conditionDrunk->cycle == 1) {
		cycle++;
	} else {
		cycle = conditionDrunk->cycle;
	}
}

bool ConditionDrunk::executeCondition(Creature* creature, int32_t interval)
{
	const int32_t r_cycle = cycle;

	bool removeCondition = true;
	creature->onTickCondition(getType(), removeCondition);

	if (r_cycle) {
		if (count <= 0) {
			count = maxCount;

			if (removeCondition) {
				cycle = r_cycle + 2 * (r_cycle <= 0) - 1;
			}
		} else {
			--count;
		}
	} else {
		return false;
	}

	return cycle > 0;
}

void ConditionDrunk::endCondition(Creature*)
{
	//
}

uint32_t ConditionDrunk::getIcons() const { return ICON_DRUNK; }
