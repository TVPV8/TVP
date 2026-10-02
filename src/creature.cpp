// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "creature.h"
#include "game.h"
#include "monster.h"
#include "configmanager.h"
#include "scheduler.h"

extern Game g_game;
extern ConfigManager g_config;
extern CreatureEvents* g_creatureEvents;

Creature::Creature() { Creature::onIdleStatus(); }

Creature::~Creature()
{
	for (Condition* condition : conditions) {
		condition->endCondition(this);
		delete condition;
	}
}

bool Creature::canSee(const Position& myPos, const Position& pos, int32_t viewRangeX, int32_t viewRangeY)
{
	if (myPos.z <= 7) {
		// we are on ground level or above (7 -> 0)
		// view is from 7 -> 0
		if (pos.z > 7) {
			return false;
		}
	} else if (myPos.z >= 8) {
		// we are underground (8 -> 15)
		// we can't see floors above 8
		if (pos.z < 8) {
			return false;
		}

		// view is +/- 2 from the floor we stand on
		if (Position::getDistanceZ(myPos, pos) > 2) {
			return false;
		}
	}

	const int_fast32_t offsetz = myPos.getZ() - pos.getZ();
	return (pos.getX() >= myPos.getX() - viewRangeX + offsetz) && (pos.getX() <= myPos.getX() + viewRangeX + offsetz) &&
	       (pos.getY() >= myPos.getY() - viewRangeY + offsetz) && (pos.getY() <= myPos.getY() + viewRangeY + offsetz);
}

bool Creature::canSee(const Position& pos) const
{
	return canSee(getPosition(), pos, Map::maxViewportX, Map::maxViewportY);
}

bool Creature::canSeeCreature(const Creature* creature) const
{
	if (!canSeeGhostMode(creature) && creature->isInGhostMode()) {
		return false;
	}

	if (!canSeeInvisibility() && creature->isInvisible()) {
		return false;
	}

	return true;
}

void Creature::setSkull(Skulls_t newSkull)
{
	skull = newSkull;
	g_game.updateCreatureSkull(this);
}

void Creature::onThink(uint32_t interval)
{
	if (followCreature && master != followCreature && !canSeeCreature(followCreature)) {
		onCreatureDisappear(followCreature, false);
	}

	if (attackedCreature && master != attackedCreature && !canSeeCreature(attackedCreature)) {
		onCreatureDisappear(attackedCreature, false);
	}

	blockTicks += interval;
	if (blockTicks >= 1000) {
		blockCount = std::min<uint32_t>(blockCount + 1, 2);
		blockTicks = 0;
	}

	// scripting event - onThink
	const CreatureEventList& thinkEvents = getCreatureEvents(CREATURE_EVENT_THINK);
	for (CreatureEvent* thinkEvent : thinkEvents) {
		thinkEvent->executeOnThink(this, interval);
	}
}

void Creature::onAttacking()
{
	if (!attackedCreature || attackedCreature->getHealth() <= 0 || attackedCreature->isRemoved() ||
	    attackedCreature->isInvisible() && !canSeeInvisibility()) {
		setAttackedCreature(nullptr);

		if (const Player* player = getPlayer()) {
			player->sendCancelTarget();
			player->sendCancelMessage("Target lost.");
		}
		return;
	}

	if (!Position::areInRange<8, 8>(attackedCreature->getPosition(), getPosition())) {
		onCreatureDisappear(attackedCreature, false);
		return;
	}

	if (!Position::areInRange<1, 1>(attackedCreature->getPosition(), getPosition())) {
		if (Player* player = getPlayer()) {
			if (player->chaseMode) {
				if (player->targetClearRound != 0 && std::time(nullptr) >= player->targetClearRound) {
					player->setAttackedCreature(nullptr);
					player->sendCancelMessage(RETURNVALUE_THEREISNOWAY);
				} else if (player->targetClearRound == 0) {
					player->targetClearRound = std::time(nullptr) + 15;
				}
				return;
			}
		} else if (Monster* monster = getMonster()) {
			if (monster->mType->info.targetDistance == 1) {
				return;
			}
		}
	}

	onAttacked();

	if (Player* player = getPlayer()) {
		player->targetClearRound = 0;
	}

	bool allowAttacking = true;
	if (const Monster* monster = getMonster()) {
		if (!monster->isHostile() && !monster->isSummon()) {
			allowAttacking = false;
		}
	}

	if (allowAttacking) {
		attackedCreature->onAttacked();
		doAttacking(); // melee and distance weapon attacks only
	}
}

void Creature::onIdleStatus()
{
	if (getHealth() > 0) {
		lastHitCreatureId = 0;
	}
}

void Creature::onCreatureAppear(Creature* creature, bool isLogin)
{
	if (creature == this) {
		if (isLogin) {
			setLastPosition(getPosition());
		}
	}
}

void Creature::onRemoveCreature(Creature* creature, bool) { onCreatureDisappear(creature, true); }

void Creature::onCreatureDisappear(const Creature* creature, bool isLogout)
{
	if (attackedCreature == creature) {
		onAttackedCreatureDisappear(isLogout);
	}

	if (followCreature == creature) {
		if (!isLogout) {
			setFollowCreature(nullptr);
		}
		onFollowCreatureDisappear(isLogout);
	}
}

void Creature::onChangeZone(ZoneType_t zone)
{
	if (attackedCreature && zone == ZONE_PROTECTION) {
		onCreatureDisappear(attackedCreature, false);
	}
}

void Creature::onAttackedCreatureChangeZone(ZoneType_t zone)
{
	if (zone == ZONE_PROTECTION) {
		onCreatureDisappear(attackedCreature, false);
	}
}

void Creature::onCreatureMove(Creature* creature, const Tile* newTile, const Position& newPos, const Tile* oldTile,
                              const Position& oldPos, bool teleport)
{
	if (creature == this) {
		if (Position::getOffsetX(oldPos, newPos) > 1 || Position::getOffsetY(oldPos, newPos) > 1 ||
		    Position::getOffsetZ(oldPos, newPos)) {
			clearToDo();
		}

		const bool diagonalStep = oldPos.z == newPos.z && oldPos.x != newPos.x && oldPos.y != newPos.y;

		int32_t waypoints = 0;
		if (currentTile) {
			if (const Item* ground = currentTile->getGround()) {
				waypoints = Item::items.getItemType(ground->getID()).speed;
			}
		}

		if (diagonalStep) {
			waypoints *= 3;
		}

		const int32_t speed = getSpeed();
		if (speed > 0) {
			earliestWalkTime = OTSYS_TIME() + 50 * ((50 + 1000 * waypoints / speed - 1) / 50);
		}

		if (newTile->getZone() != oldTile->getZone()) {
			onChangeZone(getZone());
		}
	}

	if (creature == followCreature || (creature == this && followCreature)) {
		if (newPos.z != oldPos.z || !canSee(followCreature->getPosition())) {
			onCreatureDisappear(followCreature, false);
		}
	}

	if (creature == attackedCreature || (creature == this && attackedCreature)) {
		if (newPos.z != oldPos.z || !canSee(attackedCreature->getPosition())) {
			onCreatureDisappear(attackedCreature, false);
		} else {
			if (newTile->getZone() != oldTile->getZone()) {
				onAttackedCreatureChangeZone(attackedCreature->getZone());
			}
		}
	}
}

CreatureVector Creature::getKillers() const
{
	CreatureVector killers;
	const int64_t timeNow = OTSYS_TIME();
	const uint32_t inFightTicks = g_config.getNumber(ConfigManager::PZ_LOCKED);
	for (const auto& it : damageMap) {
		if (it.CreatureID == 0) continue;
		Creature* attacker = g_game.getCreatureByID(it.CreatureID);
		if (attacker && attacker != this && timeNow - it.ticks <= inFightTicks) {
			killers.push_back(attacker);
		}
	}
	return killers;
}

void Creature::onDeath()
{
	bool lastHitUnjustified = false;
	bool mostDamageUnjustified = false;
	Creature* lastHitCreature = g_game.getCreatureByID(lastHitCreatureId);
	if (lastHitCreature) {
		lastHitUnjustified = lastHitCreature->onKilledCreature(this);
	}

	Creature* mostDamageCreature = nullptr;

	int32_t mostDamage = 0;
	std::vector<Player*> attackers;
	for (const auto& it : damageMap) {
		if (it.CreatureID == 0) continue;
		if (Creature* attacker = g_game.getCreatureByID(it.CreatureID)) {
			const CountBlock_t& cb = it;

			int64_t time = OTSYS_TIME();
			if ((time - cb.ticks) <= g_config.getNumber(ConfigManager::PZ_LOCKED)) {
				// Only count most damage creature that has done dmg within 60s
				if (cb.total > mostDamage) {
					mostDamage = cb.total;
					mostDamageCreature = attacker;
				}

				if (attacker != this) {
					if (Player* attackerPlayer = attacker->getPlayer()) {
						attackers.push_back(attackerPlayer);
					}
				}
			}
		}
	}

	distributeExperiencePoints();

	if (mostDamageCreature) {
		if (getPlayer()) {
			Player* mostDamagePlayer = mostDamageCreature->getPlayer();
			if (!mostDamagePlayer && mostDamageCreature->getMaster()) {
				mostDamagePlayer = mostDamageCreature->getMaster()->getPlayer();
			}

			if (mostDamagePlayer) {
				mostDamageUnjustified = mostDamageCreature->onKilledCreature(this, false);
				if (mostDamageUnjustified) {
					if (!g_config.getBoolean(ConfigManager::ONLY_ONE_FRAG_PER_KILL) || !lastHitUnjustified ||
					    !lastHitCreature) {
						if (lastHitCreature != mostDamagePlayer) {
							mostDamagePlayer->addUnjustifiedDead(getPlayer());
						}
					}
				}
			}
		}
	}

	for (Player* attackerPlayer : attackers) {
		attackerPlayer->removeAttacked(getPlayer());
	}

	const bool droppedCorpse =
	    dropCorpse(lastHitCreature, mostDamageCreature, lastHitUnjustified, mostDamageUnjustified);
	death(lastHitCreature);

	if (master) {
		setMaster(nullptr);
	}

	if (droppedCorpse) {
		g_game.removeCreature(this, false);
	}
}

void Creature::distributeExperiencePoints()
{
	uint64_t experience = getLostExperience();
	if (experience == 0) {
		return;
	}

	Player* thisPlayer = getPlayer();
	if (thisPlayer && !g_config.getBoolean(ConfigManager::EXPERIENCE_FROM_PLAYERS)) {
		return;
	}

	std::map<Party*, uint64_t> sharedExperience;

	for (auto& it : damageMap) {
		if (it.CreatureID == 0) continue;

		Creature* creature = g_game.getCreatureByID(it.CreatureID);
		if (!creature || creature->isRemoved()) {
			continue;
		}

		bool partySharing = false;
		const CountBlock_t& cb = it;

		int64_t gainedExperience = cb.total * experience / totalCombatDamageReceived;

		if (const Player* attackerPlayer = creature->getPlayer()) {
			if (thisPlayer) {
				const int32_t formula =
				    thisPlayer->getLevel() +
				    ((thisPlayer->getLevel() * g_config.getNumber(ConfigManager::PVP_EXP_FORMULA)) / 100);
				if (g_config.getBoolean(ConfigManager::EXPERIENCE_FROM_PLAYERS)) {
					if (attackerPlayer->getLevel() > formula ||
					    !(attackerPlayer && attackerPlayer != this && skillLoss) ||
					    attackerPlayer->getParty() && thisPlayer->getParty() == attackerPlayer->getParty()) {
						gainedExperience = 0;
					}
				} else {
					gainedExperience = 0;
				}
			}

			if (Party* party = attackerPlayer->getParty()) {
				if (party->isSharedExperienceActive() && party->isSharedExperienceEnabled()) {
					auto it = sharedExperience.find(party);
					if (it == sharedExperience.end()) {
						sharedExperience[party] = gainedExperience;
					} else {
						it->second += gainedExperience;
					}
					partySharing = true;
				}
			}
		}

		if (gainedExperience && !partySharing) {
			creature->onGainExperience(gainedExperience, this);
		}
	}

	// Takes care of sharing experience within a party
	for (auto& it : sharedExperience) {
		it.first->getLeader()->onGainExperience(it.second, this);
	}
}

bool Creature::dropCorpse(Creature* lastHitCreature, Creature* mostDamageCreature, bool lastHitUnjustified,
                          bool mostDamageUnjustified)
{
	Item* splash;
	switch (getRace()) {
		case RACE_VENOM:
			splash = Item::CreateItem(ITEM_FULLSPLASH, FLUID_SLIME);
			break;

		case RACE_BLOOD:
			splash = Item::CreateItem(ITEM_FULLSPLASH, FLUID_BLOOD);
			break;

		default:
			splash = nullptr;
			break;
	}

	Tile* tile = getTile();

	if (splash) {
		// Does not allow to create pools on tiles with a "Bottom" item
		// And remove previous splash if there is any
		if (Item* previousSplash = tile->getSplashItem()) {
			g_game.internalRemoveItem(previousSplash);
		}

		if (!tile->getItemByTopOrder(2)) {
			g_game.internalAddItem(tile, splash, INDEX_WHEREEVER, FLAG_NOLIMIT);
			g_game.startDecay(splash);
		} else {
			delete splash;
		}
	}

	Item* corpse = getCorpse(lastHitCreature, mostDamageCreature);
	if (corpse) {
		g_game.internalAddItem(tile, corpse, INDEX_WHEREEVER, FLAG_NOLIMIT);
		g_game.startDecay(corpse);
	}

	// scripting event - onDeath
	for (CreatureEvent* deathEvent : getCreatureEvents(CREATURE_EVENT_DEATH)) {
		deathEvent->executeOnDeath(this, corpse, lastHitCreature, mostDamageCreature, lastHitUnjustified,
		                           mostDamageUnjustified);
	}

	if (corpse) {
		dropLoot(corpse->getContainer(), lastHitCreature);
	}

	return true;
}

bool Creature::hasBeenAttacked(uint32_t attackerId)
{
	for (auto& it : damageMap) {
		if (it.CreatureID == attackerId) {
			return (OTSYS_TIME() - it.ticks) <= g_config.getNumber(ConfigManager::PZ_LOCKED);
		}
	}

	return false;
}

Item* Creature::getCorpse(Creature*, Creature*) { return Item::CreateItem(getLookCorpse()); }

void Creature::changeHealth(int32_t healthChange, bool sendHealthChange /* = true*/)
{
	const int32_t oldHealth = health;

	if (healthChange > 0) {
		health += std::min<int32_t>(healthChange, getMaxHealth() - health);
	} else {
		health = std::max<int32_t>(0, health + healthChange);
	}

	if (sendHealthChange && oldHealth != health) {
		g_game.addCreatureHealth(this);
	}

	if (health <= 0) {
		g_game.executeDeath(this);
	}
}

void Creature::gainHealth(Creature* healer, int32_t healthGain)
{
	changeHealth(healthGain);
	if (healer) {
		healer->onTargetCreatureGainHealth(this, healthGain);
	}
}

void Creature::drainHealth(Creature* attacker, int32_t damage)
{
	changeHealth(-damage, false);

	if (attacker) {
		attacker->onAttackedCreatureDrainHealth(this, damage);
		lastHitCreatureId = attacker->getID();
	} else {
		lastHitCreatureId = 0;
	}
}

BlockType_t Creature::blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
                               bool checkDefense /* = false */, bool checkArmor /* = false */, bool /* field = false */,
                               bool /* ignoreResistances = false */, bool meleeHit /* = false*/)
{
	BlockType_t blockType = BLOCK_NONE;

	if (isImmune(combatType)) {
		damage = 0;
		blockType = BLOCK_IMMUNITY;
	} else if (checkDefense || checkArmor) {
		bool hasDefense = false;

		if (checkDefense) {
			if (g_config.getBoolean(ConfigManager::USE_CLASSIC_COMBAT_FORMULAS)) {
				if (OTSYS_TIME() >= static_cast<int64_t>(earliestDefendTime)) {
					hasDefense = true;
					earliestDefendTime = lastDefense + 2000;
					lastDefense = OTSYS_TIME();
				}
			} else {
				if (blockCount > 0) {
					--blockCount;
					hasDefense = true;
				}
			}
		}

		if (checkDefense && hasDefense && canUseDefense) {
			const int32_t defense = getDefense();

			if (g_config.getBoolean(ConfigManager::USE_CLASSIC_COMBAT_FORMULAS)) {
				damage -= defense;
			} else {
				damage -= uniform_random(defense / 2, defense);
			}

			if (damage <= 0) {
				damage = 0;
				blockType = BLOCK_DEFENSE;
				checkArmor = false;
			}
		}

		if (checkArmor) {
			const int32_t armor = getArmor();

			if (g_config.getBoolean(ConfigManager::USE_CLASSIC_COMBAT_FORMULAS)) {
				damage -= armor;
			} else {
				if (armor > 3) {
					damage -= uniform_random(armor / 2, armor - (armor % 2 + 1));
				} else if (armor > 0) {
					--damage;
				}
			}

			if (damage <= 0) {
				damage = 0;
				blockType = BLOCK_ARMOR;
			}
		}

		if (hasDefense && blockType != BLOCK_NONE) {
			onBlockHit();
		}
	}

	if (attacker) {
		attacker->onAttackedCreature(this);
		attacker->onAttackedCreatureBlockHit(blockType, meleeHit);
	}

	if (damage > 0) {
		onAttacked();
	}

	return blockType;
}

bool Creature::setAttackedCreature(Creature* creature)
{
	Creature* previousTarget = attackedCreature;

	if (creature) {
		const Position& creaturePos = creature->getPosition();
		if (creaturePos.z != getPosition().z || !canSee(creaturePos)) {
			attackedCreature = nullptr;

			if (previousTarget) {
				previousTarget->decrementReferenceCounter();
			}
			return false;
		}

		attackedCreature = creature;
		attackedCreature->incrementReferenceCounter();

		if (getPlayer()) {
			onAttackedCreature(attackedCreature, true);
			attackedCreature->onAttacked();
		}
	} else {
		attackedCreature = nullptr;
	}

	if (previousTarget) {
		previousTarget->decrementReferenceCounter();
	}

	return true;
}

bool Creature::getRandomStep(Direction& dir, bool checkForTiles) const
{
	if (!checkForTiles) {
		switch (rand() % 3) {
			case 0:
				dir = DIRECTION_EAST;
				break;
			case 1:
				dir = DIRECTION_WEST;
				break;
			case 2:
				dir = DIRECTION_SOUTH;
				break;
			case 3:
				dir = DIRECTION_NORTH;
				break;
			default:
				return false;
		}

		return true;
	} else {
		static std::vector dirList{DIRECTION_NORTH, DIRECTION_WEST, DIRECTION_EAST, DIRECTION_SOUTH};
		std::ranges::shuffle(dirList, getRandomGenerator());

		for (const Direction currentDir : dirList) {
			const Tile* tile = g_game.map.getTile(getNextPosition(currentDir, getPosition()));
			if (!checkForTiles ||
			    tile && tile->queryAdd(INDEX_WHEREEVER, *this, 1, FLAG_IGNOREFIELDDAMAGE) == RETURNVALUE_NOERROR) {
				dir = currentDir;
				return true;
			}
		}
	}

	return false;
}

bool Creature::setFollowCreature(Creature* creature)
{
	Creature* previousTarget = followCreature;
	if (creature) {
		if (followCreature == creature) {
			return true;
		}

		const Position& creaturePos = creature->getPosition();
		if (creaturePos.z != getPosition().z || !canSee(creaturePos)) {
			followCreature = nullptr;
			return false;
		}

		creature->incrementReferenceCounter();
		followCreature = creature;
	} else {
		followCreature = nullptr;
	}

	if (previousTarget) {
		previousTarget->decrementReferenceCounter();
	}

	onFollowCreature(creature);
	return true;
}

int32_t Creature::getDamageDealtByAttacker(const Creature* attacker) const
{
	for (const auto& it : damageMap) {
		const CountBlock_t& cb = it;
		if (it.CreatureID == attacker->getID()) {
			return cb.total;
		}
	}
	return 0;
}

void Creature::addDamagePoints(const Creature* attacker, int32_t damagePoints)
{
	if (damagePoints <= 0) {
		return;
	}

	totalCombatDamageReceived += damagePoints;

	uint32_t attackerId = attacker->getID();

	for (auto& it : damageMap) {
		if (it.CreatureID == attackerId) {
			it.total += damagePoints;
			it.ticks = OTSYS_TIME();
			return;
		}
	}

	damageMap[actDamageEntry].CreatureID = attackerId;
	damageMap[actDamageEntry].total = damagePoints;
	damageMap[actDamageEntry].ticks = OTSYS_TIME();

	uint8_t nextDamageEntry = 0;
	if (actDamageEntry != CREATURE_DAMAGEMAP_SIZE - 1) {
		nextDamageEntry = actDamageEntry + 1;
	}

	actDamageEntry = nextDamageEntry;
}

void Creature::onAddCondition(ConditionType_t type)
{
	if (getNpc()) {
		if (type == CONDITION_FIRE || type == CONDITION_POISON || type == CONDITION_ENERGY ||
		    type == CONDITION_BLEEDING) {
			removeCondition(type);
			return;
		}
	}

	Condition* condition = getCondition(type);
	if (condition && type == CONDITION_POISON || type == CONDITION_FIRE || type == CONDITION_ENERGY) {
		Creature* responsible;
		if (const uint32_t owner = condition->getParam(CONDITION_PARAM_OWNER); owner == 0) {
			responsible = g_game.getPlayerByGUID(condition->getParam(CONDITION_PARAM_OWNERGUID));
		} else {
			responsible = g_game.getCreatureByID(owner);
		}

		if (responsible) {
			responsible->onAttackedCreature(this);
		}
	}

	if (type == CONDITION_PARALYZE && hasCondition(CONDITION_HASTE)) {
		removeCondition(CONDITION_HASTE);
	} else if (type == CONDITION_HASTE && hasCondition(CONDITION_PARALYZE)) {
		removeCondition(CONDITION_PARALYZE);
	} else if (type == CONDITION_LIGHT) {
		g_game.changeLight(this);
	}
}

void Creature::onAddCombatCondition(ConditionType_t)
{
	//
}

void Creature::onEndCondition(ConditionType_t)
{
	//
}

void Creature::onTickCondition(ConditionType_t type, bool& bRemove)
{
	const MagicField* field = getTile()->getFieldItem();
	if (!field) {
		return;
	}

	switch (type) {
		case CONDITION_FIRE:
			bRemove = (field->getCombatType() != COMBAT_FIREDAMAGE);
			break;
		case CONDITION_ENERGY:
			bRemove = (field->getCombatType() != COMBAT_ENERGYDAMAGE);
			break;
		case CONDITION_POISON:
			bRemove = (field->getCombatType() != COMBAT_EARTHDAMAGE);
			break;
		case CONDITION_BLEEDING:
			bRemove = (field->getCombatType() != COMBAT_PHYSICALDAMAGE);
			break;
		default:
			break;
	}
}

void Creature::onCombatRemoveCondition(Condition* condition) { removeCondition(condition); }

void Creature::onAttacked()
{
	//
}

void Creature::onAttackedCreatureDrainHealth(Creature* target, int32_t points)
{
	const Creature* master = getMaster();
	if (!master) {
		target->addDamagePoints(this, points);
	} else {
		target->addDamagePoints(this, points / 2);
		target->addDamagePoints(master, points / 2);
	}
}

bool Creature::onKilledCreature(Creature* target, bool)
{
	// Do not execute double onKill events on the same target
	if (lastKilledCreatureIdEvent == target->getID()) {
		return false;
	}

	lastKilledCreatureIdEvent = target->getID();

	if (master) {
		master->onKilledCreature(target);
	}

	// scripting event - onKill
	const CreatureEventList& killEvents = getCreatureEvents(CREATURE_EVENT_KILL);
	for (CreatureEvent* killEvent : killEvents) {
		killEvent->executeOnKill(this, target);
	}
	return false;
}

void Creature::onGainExperience(uint64_t gainExp, Creature* target)
{
	if (gainExp == 0) {
		return;
	}

	g_game.addAnimatedText(getPosition(), TEXTCOLOR_WHITE_EXP, std::to_string(gainExp));
}

bool Creature::setMaster(Creature* newMaster)
{
	if (!newMaster && !master) {
		return false;
	}

	if (newMaster) {
		incrementReferenceCounter();
		newMaster->incrementReferenceCounter();
		newMaster->summons.push_back(this);
	}

	Creature* oldMaster = master;
	master = newMaster;

	if (oldMaster) {
		const auto& summon = std::find(oldMaster->summons.begin(), oldMaster->summons.end(), this);
		if (summon != oldMaster->summons.end()) {
			oldMaster->summons.erase(summon);
			decrementReferenceCounter();
		}

		oldMaster->decrementReferenceCounter();
	}
	return true;
}

bool Creature::addCondition(Condition* condition)
{
	if (condition == nullptr) {
		return false;
	}

	if (Condition* prevCond = getCondition(condition->getType(), condition->getId(), condition->getSubId())) {
		prevCond->addCondition(this, condition);
		onAddCondition(condition->getType());
		delete condition;
		return true;
	}

	if (condition->startCondition(this)) {
		conditions.push_back(condition);
		onAddCondition(condition->getType());
		return true;
	}

	delete condition;
	return false;
}

bool Creature::addCombatCondition(Condition* condition)
{
	// Caution: condition variable could be deleted after the call to addCondition
	const ConditionType_t type = condition->getType();

	if (!addCondition(condition)) {
		return false;
	}

	onAddCombatCondition(type);
	return true;
}

void Creature::removeCondition(ConditionType_t type)
{
	auto it = conditions.begin();
	const auto& end = conditions.end();
	while (it != end) {
		Condition* condition = *it;
		if (condition->getType() != type) {
			++it;
			continue;
		}

		it = conditions.erase(it);

		condition->endCondition(this);
		delete condition;

		onEndCondition(type);
	}
}

void Creature::removeCondition(ConditionType_t type, ConditionId_t conditionId)
{
	auto it = conditions.begin();
	const auto& end = conditions.end();
	while (it != end) {
		Condition* condition = *it;
		if (condition->getType() != type || condition->getId() != conditionId) {
			++it;
			continue;
		}

		it = conditions.erase(it);

		condition->endCondition(this);
		delete condition;

		onEndCondition(type);
	}
}

void Creature::removeCombatCondition(ConditionType_t type)
{
	std::vector<Condition*> removeConditions;
	for (Condition* condition : conditions) {
		if (condition->getType() == type) {
			removeConditions.push_back(condition);
		}
	}

	for (Condition* condition : removeConditions) {
		onCombatRemoveCondition(condition);
	}
}

void Creature::removeCondition(Condition* condition)
{
	const auto& it = std::find(conditions.begin(), conditions.end(), condition);
	if (it == conditions.end()) {
		return;
	}

	conditions.erase(it);

	condition->endCondition(this);
	onEndCondition(condition->getType());
	delete condition;
}

Condition* Creature::getCondition(ConditionType_t type) const
{
	for (Condition* condition : conditions) {
		if (condition->getType() == type) {
			return condition;
		}
	}
	return nullptr;
}

Condition* Creature::getCondition(ConditionType_t type, ConditionId_t conditionId, uint32_t subId /* = 0*/) const
{
	for (Condition* condition : conditions) {
		if (condition->getType() == type && condition->getId() == conditionId && condition->getSubId() == subId) {
			return condition;
		}
	}
	return nullptr;
}

void Creature::executeConditions(uint32_t interval)
{
	const ConditionList tempConditions{conditions};
	for (Condition* condition : tempConditions) {
		auto it = std::find(conditions.begin(), conditions.end(), condition);
		if (it == conditions.end()) {
			continue;
		}

		if (!condition->executeCondition(this, interval)) {
			it = std::find(conditions.begin(), conditions.end(), condition);
			if (it != conditions.end()) {
				conditions.erase(it);
				condition->endCondition(this);
				onEndCondition(condition->getType());
				delete condition;
			}
		}
	}
}

bool Creature::hasCondition(ConditionType_t type, uint32_t subId /* = 0*/) const
{
	if (isSuppress(type)) {
		return false;
	}

	const int64_t timeNow = OTSYS_TIME();
	for (const Condition* condition : conditions) {
		if (condition->getType() != type || condition->getSubId() != subId) {
			continue;
		}

		if (condition->getEndTime() >= timeNow || condition->getTicks() == -1) {
			return true;
		}
	}
	return false;
}

bool Creature::isImmune(CombatType_t type) const
{
	return hasBitSet(static_cast<uint32_t>(type), getDamageImmunities());
}

bool Creature::isImmune(ConditionType_t type) const
{
	return hasBitSet(static_cast<uint32_t>(type), getConditionImmunities());
}

bool Creature::isSuppress(ConditionType_t type) const
{
	return hasBitSet(static_cast<uint32_t>(type), getConditionSuppressions());
}

LightInfo Creature::getCreatureLight() const
{
	LightInfo lightInfo{};

	const ConditionLight* conditionLight = dynamic_cast<ConditionLight*>(getCondition(CONDITION_LIGHT));
	if (conditionLight) {
		lightInfo = conditionLight->getLightInfo();

		int32_t red = 5 * lightInfo.level;
		int32_t green = 5 * lightInfo.level;
		int32_t blue = 5 * lightInfo.level;
		int32_t brightness = lightInfo.level;

		// Calculate RGB from brightness according to CIP
		if (lightInfo.color == 0 && brightness > 0) {
			lightInfo.color = blue / brightness + 6 * (green / brightness) + 36 * (red / brightness);
			lightInfo.blue = blue;
			lightInfo.red = red;
			lightInfo.green = green;
		}
	}

	return lightInfo;
}

bool Creature::registerCreatureEvent(const std::string& name)
{
	CreatureEvent* event = g_creatureEvents->getEventByName(name);
	if (!event) {
		return false;
	}

	const CreatureEventType_t type = event->getEventType();
	if (hasEventRegistered(type)) {
		for (const CreatureEvent* creatureEvent : eventsList) {
			if (creatureEvent == event) {
				return false;
			}
		}
	} else {
		scriptEventsBitField |= static_cast<uint32_t>(1) << type;
	}

	eventsList.push_back(event);
	return true;
}

bool Creature::unregisterCreatureEvent(const std::string& name)
{
	const CreatureEvent* event = g_creatureEvents->getEventByName(name);
	if (!event) {
		return false;
	}

	const CreatureEventType_t type = event->getEventType();
	if (!hasEventRegistered(type)) {
		return false;
	}

	bool resetTypeBit = true;

	auto it = eventsList.begin(), end = eventsList.end();
	while (it != end) {
		const CreatureEvent* curEvent = *it;
		if (curEvent == event) {
			it = eventsList.erase(it);
			continue;
		}

		if (curEvent->getEventType() == type) {
			resetTypeBit = false;
		}
		++it;
	}

	if (resetTypeBit) {
		scriptEventsBitField &= ~(static_cast<uint32_t>(1) << type);
	}
	return true;
}

CreatureEventList Creature::getCreatureEvents(CreatureEventType_t type) const
{
	CreatureEventList tmpEventList;

	if (!hasEventRegistered(type)) {
		return tmpEventList;
	}

	for (CreatureEvent* creatureEvent : eventsList) {
		if (!creatureEvent->isLoaded()) {
			continue;
		}

		if (creatureEvent->getEventType() == type) {
			tmpEventList.push_back(creatureEvent);
		}
	}

	return tmpEventList;
}

bool FrozenPathingConditionCall::isInRange(const Position& startPos, const Position& testPos,
                                           const FindPathParams& fpp) const
{
	if (fpp.fullPathSearch) {
		if (testPos.x > targetPos.x + fpp.maxTargetDist) {
			return false;
		}

		if (testPos.x < targetPos.x - fpp.maxTargetDist) {
			return false;
		}

		if (testPos.y > targetPos.y + fpp.maxTargetDist) {
			return false;
		}

		if (testPos.y < targetPos.y - fpp.maxTargetDist) {
			return false;
		}
	} else {
		const int_fast32_t dx = Position::getOffsetX(startPos, targetPos);

		const int32_t dxMax = (dx >= 0 ? fpp.maxTargetDist : 0);
		if (testPos.x > targetPos.x + dxMax) {
			return false;
		}

		const int32_t dxMin = (dx <= 0 ? fpp.maxTargetDist : 0);
		if (testPos.x < targetPos.x - dxMin) {
			return false;
		}

		const int_fast32_t dy = Position::getOffsetY(startPos, targetPos);

		const int32_t dyMax = (dy >= 0 ? fpp.maxTargetDist : 0);
		if (testPos.y > targetPos.y + dyMax) {
			return false;
		}

		const int32_t dyMin = (dy <= 0 ? fpp.maxTargetDist : 0);
		if (testPos.y < targetPos.y - dyMin) {
			return false;
		}
	}
	return true;
}

bool FrozenPathingConditionCall::operator()(const Position& startPos, const Position& testPos,
                                            const FindPathParams& fpp, int32_t& bestMatchDist) const
{
	if (!isInRange(startPos, testPos, fpp)) {
		return false;
	}

	if (fpp.clearSight && !g_game.canThrowObjectTo(testPos, targetPos, false)) {
		return false;
	}

	const int32_t testDist =
	    std::max<int32_t>(Position::getDistanceX(targetPos, testPos), Position::getDistanceY(targetPos, testPos));
	if (fpp.maxTargetDist == 1) {
		if (testDist < fpp.minTargetDist || testDist > fpp.maxTargetDist) {
			return false;
		}

		return true;
	} else if (testDist <= fpp.maxTargetDist) {
		if (testDist < fpp.minTargetDist) {
			return false;
		}

		if (testDist == fpp.maxTargetDist) {
			bestMatchDist = 0;
			return true;
		} else if (testDist > bestMatchDist) {
			// not quite what we want, but the best so far
			bestMatchDist = testDist;
			return true;
		}
	}
	return false;
}

bool Creature::isInvisible() const { return hasCondition(CONDITION_INVISIBLE); }

void Creature::onWalk(Direction& dir, uint32_t& flags)
{
	if (const ConditionDrunk* drunkCondition = dynamic_cast<ConditionDrunk*>(getCondition(CONDITION_DRUNK))) {
		const bool isDrunkSuppressed = isImmune(CONDITION_DRUNK) || isSuppress(CONDITION_DRUNK);
		if (!isDrunkSuppressed && drunkCondition->getCycle() > 0 &&
		    !(rand() % std::max<int32_t>(1, (7 - drunkCondition->getCycle())))) {
			const bool drunkStep = getRandomStep(dir, false);
			const bool cancelWalk = clearToDo();
			if (cancelWalk) {
				g_game.internalCreatureSay(this, getMonster() ? TALKTYPE_MONSTER_SAY : TALKTYPE_SAY, "Hicks!", false);

				if (const Player* player = getPlayer()) {
					player->sendCancelWalk();
				}

				if (getNpc() || getMonster()) {
					addWaitToDo(100);
					startToDo();
				}
			}
		}
	}
}

bool Creature::getPathTo(const Position& targetPos, std::vector<Direction>& dirList, const FindPathParams& fpp)
{
	return g_game.map.getPathMatching(*this, dirList, FrozenPathingConditionCall(targetPos), fpp);
}

bool Creature::getPathTo(const Position& targetPos, std::vector<Direction>& dirList, int32_t minTargetDist,
                         int32_t maxTargetDist, bool fullPathSearch /*= true*/, bool clearSight /*= true*/,
                         int32_t maxSearchDist /*= 0*/)
{
	FindPathParams fpp;
	fpp.fullPathSearch = fullPathSearch;
	fpp.maxSearchDist = maxSearchDist;
	fpp.clearSight = clearSight;
	fpp.minTargetDist = minTargetDist;
	fpp.maxTargetDist = maxTargetDist;
	return getPathTo(targetPos, dirList, fpp);
}

int64_t Creature::calculateToDoDelay()
{
	const ToDoEntry& toDoEntry = toDoEntries[currentToDo];
	const int64_t now = OTSYS_TIME();

	if (toDoEntry.type == TODO_USEEX) {
		if (const Player* player = getPlayer()) {
			if (now < player->earliestMultiUseTime) {
				return player->earliestMultiUseTime - now;
			}
		}
	} else if (toDoEntry.type == TODO_WALK) {
		if (now < earliestWalkTime) {
			return earliestWalkTime - now;
		}
	} else if (toDoEntry.type == TODO_WAIT) {
		if (now >= earliestWalkTime && now >= toDoEntry.time) {
			return 0;
		}

		int64_t time = toDoEntry.time;
		if (earliestWalkTime >= time) {
			time = earliestWalkTime;
		}

		return time - now;
	} else if (toDoEntry.type == TODO_ATTACK) {
		if (const Player* player = getPlayer()) {
			int64_t checkTime = player->earliestAttackTime;
			if (now < checkTime) {
				checkTime = player->earliestSpellTime;
			} else {
				checkTime = player->earliestSpellTime;
				if (now >= checkTime) {
					return 0;
				}
			}

			if (player->earliestAttackTime - now >= checkTime - now) {
				return player->earliestAttackTime - now;
			}

			return player->earliestSpellTime - now;
		}

		if (const Monster* monster = getMonster()) {
			if (now >= monster->earliestMeleeAttack) {
				return 0;
			}

			return monster->earliestMeleeAttack - now;
		}
	}

	return 0;
}

void Creature::addYieldToDo()
{
	if (isExecuting) {
		// no need to activate the creature
		return;
	}

	// This just once again demonstrates why we need to get rid of dispatcher thread
	addWaitToDo(50);
	startToDo();
}

void Creature::addWaitToDo(int32_t delay)
{
	if (isExecuting && clearToDo()) {
		if (const Player* player = getPlayer()) {
			player->sendCancelWalk();
		}
	}

	ToDoEntry toDoEntry;
	totalToDo++;
	toDoEntry.type = TODO_WAIT;
	toDoEntry.time = OTSYS_TIME() + delay;
	toDoEntries.push_back(toDoEntry);
}

void Creature::addWalkToDo(const std::vector<Direction>& dirList, int32_t maxSteps)
{
	int32_t steps = 0;
	for (const Direction& dir : dirList) {
		addWalkToDo(dir);
		if (maxSteps != -1 && ++steps >= maxSteps) {
			break;
		}
	}
}

void Creature::addWalkToDo(Direction dir)
{
	if (isExecuting && clearToDo()) {
		if (const Player* player = getPlayer()) {
			player->sendCancelWalk();
		}
	}

	ToDoEntry toDoEntry;
	totalToDo++;
	toDoEntry.type = TODO_WALK;
	toDoEntry.function = std::bind(&Game::moveCreature, &g_game, this, dir, FLAG_IGNOREFIELDDAMAGE);
	toDoEntries.push_back(toDoEntry);
}

void Creature::addAttackToDo()
{
	if (isExecuting && clearToDo()) {
		if (const Player* player = getPlayer()) {
			player->sendCancelWalk();
		}
	}

	/*if (Monster* monster = getMonster()) {
	    std::cout << "ChaseMode: " << monster->chaseMode << std::endl;
	    if (monster->chaseMode && monster->attackedCreature) {
	        const Position& targetPos = monster->attackedCreature->getPosition();
	        int32_t distance = std::max<int32_t>(Position::getDistanceX(targetPos, getPosition()),
	Position::getDistanceY(targetPos, getPosition()));

	        if (distance > 1) {
	            std::cout << "Distance: " << distance << std::endl;

	            std::vector<Direction> dirList;
	            if (getPathTo(targetPos, dirList, 0, 1, true, true))
	                addWalkToDo(dirList, 3);
	        }
	    }
	}*/

	ToDoEntry toDoEntry;
	totalToDo++;
	toDoEntry.type = TODO_ATTACK;
	toDoEntries.push_back(toDoEntry);
}

void Creature::addActionToDo(ToDoType_t type, std::function<void(void)>&& function)
{
	if (isExecuting && clearToDo()) {
		if (const Player* player = getPlayer()) {
			player->sendCancelWalk();
		}
	}

	ToDoEntry toDoEntry;
	totalToDo++;
	toDoEntry.type = type;
	toDoEntry.function = std::move(function);
	toDoEntries.push_back(toDoEntry);
}

void Creature::addActionToDo(std::function<void(void)>&& function)
{
	if (isExecuting && clearToDo()) {
		if (const Player* player = getPlayer()) {
			player->sendCancelWalk();
		}
	}

	ToDoEntry toDoEntry;
	totalToDo++;
	toDoEntry.type = TODO_ACTION;
	toDoEntry.function = std::move(function);
	toDoEntries.push_back(toDoEntry);
}

void Creature::stopToDo()
{
	if (isExecuting) {
		stopExecuting = true;
	} else if (const Player* player = getPlayer()) {
		player->sendCancelWalk();
	}
}

bool Creature::clearToDo()
{
	bool cancelWalk = false;
	for (const ToDoEntry& entry : toDoEntries) {
		if (entry.type == TODO_WALK) {
			cancelWalk = true;
		}
	}

	toDoEntries.clear();
	isExecuting = false;
	currentToDo = 0;
	totalToDo = 0;
	stopExecuting = false;
	return cancelWalk;
}

void Creature::startToDo()
{
	if (totalToDo == 0) {
		return;
	}

	isExecuting = true;
	currentToDo = 0;

	const int64_t delay = calculateToDoDelay();
	earliestWakeUpTime = OTSYS_TIME() + delay;
	if (delay > 50) {
		g_scheduler.addEvent(
		    createSchedulerTask(static_cast<uint32_t>(delay), std::bind(&Game::executeCreature, &g_game, getID())));
	} else {
		g_scheduler.addEvent(createSchedulerTask(50, std::bind(&Game::executeCreature, &g_game, getID())));
	}
}

void Creature::executeToDoEntries()
{
	while (isExecuting && !isRemoved() && earliestWakeUpTime <= OTSYS_TIME()) {
		if (currentToDo >= totalToDo) {
			clearToDo();
			onIdleStimulus();
			return;
		}

		const int64_t delay = calculateToDoDelay();
		if (delay > 0) {
			if (stopExecuting) {
				clearToDo();
				if (const Player* player = getPlayer()) {
					player->sendCancelWalk();
				}
			} else {
				earliestWakeUpTime = OTSYS_TIME() + delay;
				if (delay > 50) {
					g_scheduler.addEvent(createSchedulerTask(static_cast<uint32_t>(delay),
					                                         std::bind(&Game::executeCreature, &g_game, getID())));
				} else {
					g_scheduler.addEvent(createSchedulerTask(50, std::bind(&Game::executeCreature, &g_game, getID())));
				}
			}

			return;
		}

		ToDoEntry& toDoEntry = toDoEntries[currentToDo];
		currentToDo++;

		if (toDoEntry.type >= TODO_ACTION) {
			if (toDoEntry.function) {
				toDoEntry.function();
			}
		} else if (toDoEntry.type == TODO_ATTACK) {
			onAttacking();
		}

		if (stopExecuting) {
			clearToDo();
			if (const Player* player = getPlayer()) {
				player->sendCancelWalk();
			}
			return;
		}
	}
}
