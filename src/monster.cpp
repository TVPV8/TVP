// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "monster.h"
#include "game.h"
#include "spells.h"
#include "events.h"
#include "configmanager.h"
#include "weapons.h"

extern Game g_game;
extern Monsters g_monsters;
extern Events* g_events;
extern ConfigManager g_config;

uint32_t Monster::monsterAutoID = 0x40000000;

Monster* Monster::createMonster(const std::string& name, const std::vector<LootBlock>* extraLoot /* = nullptr*/)
{
	MonsterType* mType = g_monsters.getMonsterType(name);
	if (!mType) {
		return nullptr;
	}
	return new Monster(mType, extraLoot);
}

void Monster::addMonsterItemInventory(Container* bagItem, Item* item)
{
	const ItemType& itemType = Item::items.getItemType(item->getID());
	const WeaponType_t weaponType = itemType.weaponType;
	if (weaponType == WEAPON_AXE ||
		weaponType == WEAPON_CLUB ||
		weaponType == WEAPON_SWORD ||
		weaponType == WEAPON_SHIELD ||
		weaponType == WEAPON_WAND ||
		weaponType == WEAPON_DISTANCE ||
		itemType.decayTime > 0 ||
		itemType.charges > 0 ||
		itemType.stopTime) {
		if (bagItem->size() < bagItem->capacity()) {
			bagItem->addItemFront(item);
		} else {
			delete item;
		}
	} else {
		if (item->getSlotPosition() & SLOTP_HEAD && !inventory[CONST_SLOT_HEAD]) {
			inventory[CONST_SLOT_HEAD] = item;
		} else if (item->getSlotPosition() & SLOTP_NECKLACE && !inventory[CONST_SLOT_NECKLACE]) {
			inventory[CONST_SLOT_NECKLACE] = item;
		} else if (item->getSlotPosition() & SLOTP_ARMOR && !inventory[CONST_SLOT_ARMOR]) {
			inventory[CONST_SLOT_ARMOR] = item;
		} else if (item->getSlotPosition() & SLOTP_HAND && !inventory[CONST_SLOT_RIGHT]) {
			inventory[CONST_SLOT_RIGHT] = item;
		} else if (item->getSlotPosition() & SLOTP_HAND && !inventory[CONST_SLOT_LEFT]) {
			inventory[CONST_SLOT_LEFT] = item;
		} else if (item->getSlotPosition() & SLOTP_LEGS && !inventory[CONST_SLOT_LEGS]) {
			inventory[CONST_SLOT_LEGS] = item;
		} else if (item->getSlotPosition() & SLOTP_FEET && !inventory[CONST_SLOT_FEET]) {
			inventory[CONST_SLOT_FEET] = item;
		} else if (item->getSlotPosition() & SLOTP_RING && !inventory[CONST_SLOT_RING]) {
			inventory[CONST_SLOT_RING] = item;
		} else if (item->getSlotPosition() & SLOTP_AMMO && !inventory[CONST_SLOT_AMMO]) {
			inventory[CONST_SLOT_AMMO] = item;
		} else {
			if (bagItem->size() < bagItem->capacity()) {
				bagItem->addItemFront(item);
			} else {
				delete item;
			}
		}
	}
}

Monster::Monster(MonsterType* mType, const std::vector<LootBlock>* extraLoot /* = nullptr*/) :
	Creature(),
	nameDescription(mType->nameDescription),
	mType(mType)
{
	defaultOutfit = mType->info.outfit;
	currentOutfit = mType->info.outfit;
	skull = mType->info.skull;
	health = mType->info.health;
	healthMax = mType->info.healthMax;
	baseSpeed = mType->info.baseSpeed;
	internalLight = mType->info.light;
	hiddenHealth = mType->info.hiddenHealth;
	currentSkill = mType->info.baseSkill;
	skillFactorPercent = mType->info.skillFactorPercent;
	skillNextLevel = mType->info.skillNextLevel;
	direction = DIRECTION_NORTH;

	// register creature events
	for (const std::string& scriptName : mType->info.scripts) {
		if (!registerCreatureEvent(scriptName)) {
			std::cout << "[Warning - Monster::Monster] Unknown event name: " << scriptName << std::endl;
		}
	}

	if (!g_config.getBoolean(ConfigManager::MONSTERS_SPAWN_WITH_LOOT)) {
		return;
	}

	// inventory loot generation
	Container* bagItem = Item::CreateItem(1987, 1)->getContainer();
	if (!bagItem) {
		return;
	}

	const int32_t configRate = g_config.getNumber(ConfigManager::RATE_LOOT);
	for (auto lootInfo = mType->info.lootItems.rbegin(); lootInfo != mType->info.lootItems.rend(); ++lootInfo) {
		const int32_t lootrate = lootInfo->chance * (configRate > 0 ? configRate : 1);

		if (uniform_random(0, MAX_LOOTCHANCE) <= lootrate) {
			Item* item = Item::CreateItem(lootInfo->id, static_cast<uint16_t>(random(1, static_cast<int32_t>(lootInfo->countmax))));
			if (!item) {
				continue;
			}

			const ItemType& itemType = Item::items.getItemType(lootInfo->id);
			if (itemType.charges > 0) {
				item->setCharges(static_cast<uint16_t>(itemType.charges));
			}

			if (itemType.isFluidContainer()) {
				item->setSubType(FLUID_NONE);
			}

			addMonsterItemInventory(bagItem, item);
		}
	}

	if (extraLoot) {
		for (auto& lootInfo : *extraLoot) {
			const int32_t lootrate = lootInfo.chance * (configRate > 0 ? configRate : 1);

			if (uniform_random(0, MAX_LOOTCHANCE) <= lootrate) {
				Item* item = Item::CreateItem(lootInfo.id, static_cast<uint16_t>(random(1, static_cast<int32_t>(lootInfo.countmax))));
				if (!item) {
					continue;
				}

				const ItemType& itemType = Item::items.getItemType(lootInfo.id);
				if (itemType.charges > 0) {
					item->setCharges(static_cast<uint16_t>(itemType.charges));
				}

				if (itemType.isFluidContainer()) {
					item->setSubType(FLUID_NONE);
				}

				addMonsterItemInventory(bagItem, item);
			}
		}
	}

	if (bagItem->getItemHoldingCount() != 0) {
		inventory[CONST_SLOT_BACKPACK] = bagItem;
	} else {
		bagItem->decrementReferenceCounter();
	}
}

Monster::~Monster()
{
	if (isSummon()) {
		if (master) {
			master->decrementReferenceCounter();
			master = nullptr;
		}
	}

	for (int32_t slot = CONST_SLOT_FIRST; slot <= CONST_SLOT_LAST; slot++) {
		if (Item* inventoryItem = inventory[slot]) {
			inventoryItem->decrementReferenceCounter();
		}
	}
}

void Monster::addList()
{
	g_game.addMonster(this);
}

void Monster::removeList()
{
	g_game.removeMonster(this);
}

const std::string& Monster::getName() const
{
	if (name.empty()) {
		return mType->name;
	}
	return name;
}

void Monster::setName(const std::string& newName)
{
	if (getName() == newName) {
		return;
	}

	this->name = newName;

	// NOTE: Due to how client caches known creatures,
	// it is not feasible to send creature update to everyone that has ever met it
	SpectatorVec spectators;
	g_game.map.getSpectators(spectators, position, true, true);
	for (Creature* spectator : spectators) {
		if (Player* tmpPlayer = spectator->getPlayer()) {
			tmpPlayer->sendUpdateTileCreature(this);
		}
	}
}

const std::string& Monster::getNameDescription() const
{
	if (nameDescription.empty()) {
		return mType->nameDescription;
	}
	return nameDescription;
}

bool Monster::canSee(const Position& pos) const
{
	if (pos.z != getPosition().z) {
		// can only see same floor
		return false;
	}

	return Creature::canSee(getPosition(), pos, 10, 10);
}

bool Monster::canWalkOnFieldType(CombatType_t combatType) const
{
	switch (combatType) {
		case COMBAT_ENERGYDAMAGE:
			return mType->info.canWalkOnEnergy;
		case COMBAT_FIREDAMAGE:
			return mType->info.canWalkOnFire;
		case COMBAT_EARTHDAMAGE:
			return mType->info.canWalkOnPoison;
		default:
			return true;
	}
}

void Monster::onWalk(Direction& dir, uint32_t& flags)
{
	Creature::onWalk(dir, flags);

	flags |= FLAG_PATHFINDING | FLAG_IGNOREFIELDDAMAGE;
}

void Monster::onCreatureAppear(Creature* creature, bool isLogin)
{
	Creature::onCreatureAppear(creature, isLogin);

	if (mType->info.creatureAppearEvent != -1) {
		// onCreatureAppear(self, creature)
		LuaScriptInterface* scriptInterface = mType->info.scriptInterface;
		if (!scriptInterface->reserveScriptEnv()) {
			std::cout << "[Error - Monster::onCreatureAppear] Call stack overflow" << std::endl;
			return;
		}

		ScriptEnvironment* env = scriptInterface->getScriptEnv();
		env->setScriptId(mType->info.creatureAppearEvent, scriptInterface);

		lua_State* L = scriptInterface->getLuaState();
		scriptInterface->pushFunction(mType->info.creatureAppearEvent);

		LuaScriptInterface::pushUserdata<Monster>(L, this);
		LuaScriptInterface::setMetatable(L, -1, "Monster");

		LuaScriptInterface::pushUserdata<Creature>(L, creature);
		LuaScriptInterface::setCreatureMetatable(L, -1, creature);

		if (scriptInterface->callFunction(2)) {
			return;
		}
	}

	if (creature == this) {
		addYieldToDo();
	} else {
		onCreatureEnter(creature);
		addYieldToDo();
	}
}

void Monster::onRemoveCreature(Creature* creature, bool isLogout)
{
	Creature::onRemoveCreature(creature, isLogout);

	if (mType->info.creatureDisappearEvent != -1) {
		// onCreatureDisappear(self, creature)
		LuaScriptInterface* scriptInterface = mType->info.scriptInterface;
		if (!scriptInterface->reserveScriptEnv()) {
			std::cout << "[Error - Monster::onCreatureDisappear] Call stack overflow" << std::endl;
			return;
		}

		ScriptEnvironment* env = scriptInterface->getScriptEnv();
		env->setScriptId(mType->info.creatureDisappearEvent, scriptInterface);

		lua_State* L = scriptInterface->getLuaState();
		scriptInterface->pushFunction(mType->info.creatureDisappearEvent);

		LuaScriptInterface::pushUserdata<Monster>(L, this);
		LuaScriptInterface::setMetatable(L, -1, "Monster");

		LuaScriptInterface::pushUserdata<Creature>(L, creature);
		LuaScriptInterface::setCreatureMetatable(L, -1, creature);

		if (scriptInterface->callFunction(2)) {
			return;
		}
	}

	if (creature == this) {
		if (spawn) {
			spawn->decreaseMonsterCount();
			spawn->startSpawnCheck(Spawns::calculateSpawnDelay(spawnInterval));
		}

		setIdle(true);

		if (isRaidBoss && raidEvent && health <= 0) {
			// reschedule this raid if we have died.
			raidEvent->reschedule();
		}
	} else {
		onCreatureLeave(creature);
	}
}

void Monster::onCreatureMove(Creature* creature, const Tile* newTile, const Position& newPos,
                             const Tile* oldTile, const Position& oldPos, bool teleport)
{
	Creature::onCreatureMove(creature, newTile, newPos, oldTile, oldPos, teleport);

	if (mType->info.creatureMoveEvent != -1) {
		// onCreatureMove(self, creature, oldPosition, newPosition)
		LuaScriptInterface* scriptInterface = mType->info.scriptInterface;
		if (!scriptInterface->reserveScriptEnv()) {
			std::cout << "[Error - Monster::onCreatureMove] Call stack overflow" << std::endl;
			return;
		}

		ScriptEnvironment* env = scriptInterface->getScriptEnv();
		env->setScriptId(mType->info.creatureMoveEvent, scriptInterface);

		lua_State* L = scriptInterface->getLuaState();
		scriptInterface->pushFunction(mType->info.creatureMoveEvent);

		LuaScriptInterface::pushUserdata<Monster>(L, this);
		LuaScriptInterface::setMetatable(L, -1, "Monster");

		LuaScriptInterface::pushUserdata<Creature>(L, creature);
		LuaScriptInterface::setCreatureMetatable(L, -1, creature);

		LuaScriptInterface::pushPosition(L, oldPos);
		LuaScriptInterface::pushPosition(L, newPos);

		if (scriptInterface->callFunction(4)) {
			return;
		}
	}

	if (creature == this) {
		if (State == STATE::SLEEPING) {
			State = STATE::IDLE;
			addYieldToDo();
			return;
		}

		/*if (attackedCreature) {
			const Position& followPosition = attackedCreature->getPosition();
			const Position& position = getPosition();

			const int32_t offset_x = Position::getDistanceX(followPosition, position);
			const int32_t offset_y = Position::getDistanceY(followPosition, position);
			if ((offset_x > 1 || offset_y > 1) && mType->info.changeTargetChance > 0) {
				const Direction dir = getDirectionTo(position, followPosition);
				const Position& checkPosition = getNextPosition(dir, position);

				if (const Tile* tile = g_game.map.getTile(checkPosition)) {
					const Creature* topCreature = tile->getTopCreature();
					if (topCreature && attackedCreature != topCreature && isOpponent(topCreature)) {
						setAttackedCreature(nullptr);
					}
				}
			}
		}*/

		addYieldToDo();
	} else {
		const bool canSeeNewPos = canSee(newPos);
		const bool canSeeOldPos = canSee(oldPos);

		if (canSeeNewPos && !canSeeOldPos) {
			onCreatureEnter(creature);
		} else if (!canSeeNewPos && canSeeOldPos) {
			onCreatureLeave(creature);
		}

		addYieldToDo();

		if (creature == attackedCreature) {
			if (isExecuting && currentToDo < totalToDo && toDoEntries[currentToDo].type == TODO_ATTACK) {
				const int64_t now = OTSYS_TIME();
				if (now < earliestMeleeAttack && earliestMeleeAttack - now > 200) {
					if (newPos.z != getPosition().z || !Position::areInRange<1, 1>(getPosition(), newPos)) {
						clearToDo();
						if (mType->info.targetDistance == 1) {
							addWaitToDo(100);
						}
						startToDo();
					}
				}
			}
		}

		if (!creature->getNpc() && (!creature->getMonster() || isSummon() && master->getPlayer())) {
			State = STATE::IDLE;
			addYieldToDo();
		}
	}
}

void Monster::onCreatureSay(Creature* creature, SpeakClasses type, const std::string& text)
{
	Creature::onCreatureSay(creature, type, text);

	if (mType->info.creatureSayEvent != -1) {
		// onCreatureSay(self, creature, type, message)
		LuaScriptInterface* scriptInterface = mType->info.scriptInterface;
		if (!scriptInterface->reserveScriptEnv()) {
			std::cout << "[Error - Monster::onCreatureSay] Call stack overflow" << std::endl;
			return;
		}

		ScriptEnvironment* env = scriptInterface->getScriptEnv();
		env->setScriptId(mType->info.creatureSayEvent, scriptInterface);

		lua_State* L = scriptInterface->getLuaState();
		scriptInterface->pushFunction(mType->info.creatureSayEvent);

		LuaScriptInterface::pushUserdata<Monster>(L, this);
		LuaScriptInterface::setMetatable(L, -1, "Monster");

		LuaScriptInterface::pushUserdata<Creature>(L, creature);
		LuaScriptInterface::setCreatureMetatable(L, -1, creature);

		lua_pushnumber(L, type);
		LuaScriptInterface::pushString(L, text);

		scriptInterface->callVoidFunction(4);
	}
}

void Monster::addSkillPoint()
{
	if (skillLearningPoints == 0 || skillFactorPercent <= 999) {
		return;
	}

	skillCurrentExp++;
	if (skillCurrentExp < skillNextLevel) {
		return;
	}
	skillCurrentExp = 0;

	const int32_t delta = skillNextLevel;

	currentSkill += mType->info.skillAddCount;

	if (skillFactorPercent <= 1049) {
		skillNextLevel = delta * (currentSkill + 2 - mType->info.baseSkill);
		return;
	}

	const double factor = skillFactorPercent / 1000.0;
	double pow;

	if (static_cast<int32_t>(currentSkill + 2 - mType->info.baseSkill) < 0) {
		pow = 1.0 / std::pow(factor, mType->info.baseSkill - currentSkill + 2);
	} else {
		pow = std::pow(factor, currentSkill + 2 - mType->info.baseSkill);
	}

	const double formula = (pow / 1.0) / (factor / 1.0) * delta;
	skillNextLevel = static_cast<uint32_t>(formula);
}

void Monster::onCreatureFound(Creature* creature, bool pushFront/* = false*/)
{
	if (!creature) {
		return;
	}

	if (!canSee(creature->getPosition())) {
		return;
	}

	addYieldToDo();
}

void Monster::onCreatureEnter(Creature* creature)
{
	onCreatureFound(creature, true);
}

bool Monster::isOpponent(const Creature* creature) const
{
	if (isSummon() && getMaster()->getPlayer()) {
		if (creature == getMaster()) {
			return false;
		}
	} else {
		if (creature->isSummon()) {
			if (creature->master->getMonster()) {
				return false;
			}
		} else if (creature->getMonster()) {
			// monsters cannot attack eachother
			return false;
		}

		if (creature->getNpc()) {
			return false;
		}
	}

	if (const Player* player = creature->getPlayer()) {
		if (player->isInGhostMode()) {
			return false; // ignore ghost mode GMs walking around
		}
	}
	return true;
}

bool Monster::isCreatureAvoidable(const Creature* creature) const
{
	if (const Monster* monster = creature->getMonster()) {
		if (!canPushCreatures()) {
			return false;
		}

		if (!monster->isPushable()) {
			return false;
		}
	} else if (const Player* player = creature->getPlayer()) {
		if (!player->isInGhostMode() && player != master) {
			return false;
		}
	} 

	return true;
}

void Monster::onCreatureLeave(Creature* creature)
{
	if (creature == Target && creature->getPosition().z == Target->getPosition().z) {
		Target = nullptr;
	}
}

BlockType_t Monster::blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
                              bool checkDefense /* = false*/, bool checkArmor /* = false*/, bool /* field = false */, bool /* ignoreResistances = false */, bool meleeHit /*= false*/)
{
	BlockType_t blockType = Creature::blockHit(attacker, combatType, damage, checkDefense, checkArmor, false, false, meleeHit);

	if (damage != 0) {
		int32_t elementMod = 0;
		const auto& it = mType->info.elementMap.find(combatType);
		if (it != mType->info.elementMap.end()) {
			elementMod = it->second;
		}

		if (elementMod != 0) {
			damage = static_cast<int32_t>(std::round(damage * ((100 - elementMod) / 100.)));
			if (damage <= 0) {
				damage = 0;
				blockType = BLOCK_ARMOR;
			}
		}
	}

	return blockType;
}

bool Monster::isTarget(const Creature* creature) const
{
	if (creature->isRemoved() || !creature->isAttackable()) {
		return false;
	}

	if (creature->getPosition().z != getPosition().z) {
		return false;
	}

	return true;
}

bool Monster::selectTarget(Creature* creature)
{
	if (!isTarget(creature) || creature->getZone() == ZONE_PROTECTION) {
		return false;
	}

	return setAttackedCreature(creature);
}

void Monster::setIdle(bool idle)
{
	if (isRemoved() || getHealth() <= 0) {
		return;
	}

	isIdle = idle;

	if (!isIdle) {
		g_game.addCreatureCheck(this);
	} else {
		onIdleStatus();
		Game::removeCreatureCheck(this);
	}
}

void Monster::onAddCondition(ConditionType_t type)
{
	if (type == CONDITION_FIRE && isImmune(COMBAT_FIREDAMAGE)) {
		removeCondition(CONDITION_FIRE);
	}

	if (type == CONDITION_POISON && isImmune(COMBAT_EARTHDAMAGE)) {
		removeCondition(CONDITION_POISON);
	}

	if (type == CONDITION_ENERGY && isImmune(COMBAT_ENERGYDAMAGE)) {
		removeCondition(CONDITION_ENERGY);
	}

	if (type == CONDITION_DRUNK && (isImmune(CONDITION_DRUNK) || isImmune(CONDITION_PARALYZE))) {
		removeCondition(CONDITION_DRUNK);
	}

	if (State == STATE::SLEEPING || State == STATE::IDLE)
		setIdle(false);
}

void Monster::onEndCondition(ConditionType_t type)
{
	if (conditions.empty()) {
		if (State == STATE::SLEEPING || State == STATE::IDLE)
			setIdle(true);
	}
}

void Monster::onAttackedCreature(Creature* creature, bool addInFightTicks /* = true */)
{
	Creature::onAttackedCreature(creature, addInFightTicks);

	if (isSummon()) {
		master->onAttackedCreature(creature, addInFightTicks);
	}
}

void Monster::onAttackedCreatureBlockHit(BlockType_t blockType, bool meleeHit /* = false */)
{
	switch (blockType) {
		case BLOCK_NONE: {
			// This function is called for any damage being dealt, so only reset skill points from a melee attack
			if (meleeHit) {
				skillLearningPoints = 30;
			}
			break;
		}

		case BLOCK_IMMUNITY:
		case BLOCK_DEFENSE:
		case BLOCK_ARMOR: {
			//need to draw blood every 30 hits
			if (skillLearningPoints > 0) {
				--skillLearningPoints;
			}
			break;
		}

		default: {
			break;
		}
	}
}

void Monster::onAttackedCreatureDrainHealth(Creature* target, int32_t points)
{
	Creature::onAttackedCreatureDrainHealth(target, points);

	if (master) {
		if (Player* player = master->getPlayer()) {
			if (player->getParty() && player->getParty()->isSharedExperienceActive()) {
				Monster* tmpMonster = target->getMonster();
				if (tmpMonster && tmpMonster->isHostile()) {
					//We have fulfilled a requirement for shared experience
					player->getParty()->updatePlayerTicks(player, points);
				}
			}
		}
	}
}

void Monster::onIdleStimulus()
{
	if (isExecuting || isRemoved() || getHealth() <= 0) {
		return;
	}

	if (mType->info.creatureIdleEvent != -1) {
		// onIdleStimulus(self)
		LuaScriptInterface* scriptInterface = mType->info.scriptInterface;
		if (!scriptInterface->reserveScriptEnv()) {
			std::cout << "[Error - Monster::onIdleStimulus] Call stack overflow" << std::endl;
			return;
		}

		ScriptEnvironment* env = scriptInterface->getScriptEnv();
		env->setScriptId(mType->info.creatureIdleEvent, scriptInterface);

		lua_State* L = scriptInterface->getLuaState();
		scriptInterface->pushFunction(mType->info.creatureIdleEvent);

		LuaScriptInterface::pushUserdata<Monster>(L, this);
		LuaScriptInterface::setMetatable(L, -1, "Monster");

		if (scriptInterface->callFunction(1)) {
			checkVoices();
			doAttackSpells();
			doDefensiveSpells();
			return;
		}
	}

	pathBlockCheck = false;

	if (g_config.getBoolean(ConfigManager::REMOVE_ON_DESPAWN) && 
		!isInSpawnRange(getPosition()) || 
		(lifeTimeExpiration > 0 && OTSYS_TIME() >= static_cast<int64_t>(lifeTimeExpiration))) {
		g_game.addMagicEffect(this->getPosition(), CONST_ME_POFF);
		g_game.removeCreature(this);
		return;
	}

	// monsters are considered outside of their spawn radius in a 8x8 range
	if (getMasterPos().x != 0 && !Position::areInRange<8, 8, 0>(getPosition(), getMasterPos())) {
		if (spawn && isHostile() && g_config.getBoolean(ConfigManager::ALLOW_MONSTER_OVERSPAWN)) {
			spawn->decreaseMonsterCount();
			spawn->startSpawnCheck(Spawns::calculateSpawnDelay(spawnInterval));
			originalSpawn = spawn;
			spawn = nullptr;
		}
	} else if (!spawn && originalSpawn) { // we returned to our spawn range
		spawn = originalSpawn;
		spawn->increaseMonsterCount();
		originalSpawn = nullptr;
	}

	if (isSummon()) {
		if (!master) {
			State = STATE::SLEEPING;
			setIdle(true);
			return;
		}

		int32_t dx = Position::getDistanceX(getPosition(), master->getPosition());
		int32_t dy = Position::getDistanceY(getPosition(), master->getPosition());

		if (master->isRemoved() || dx > 30 || dy > 30 || (getPosition().z != master->getPosition().z && !master->getPlayer())) {
			if (master->getPlayer()) {
				g_game.removeCreature(this);
				g_game.addMagicEffect(getPosition(), CONST_ME_POFF);
			} else {
				changeHealth(-getMaxHealth());
			}

			State = STATE::SLEEPING;
			setIdle(true);
			return;
		}

		setAttackedCreature(master->attackedCreature);

		if (master->attackedCreature == this || !master->attackedCreature) {
			setAttackedCreature(master);
		}
	}

	if (attackedCreature) {
		int32_t dx = Position::getDistanceX(getPosition(), attackedCreature->getPosition());
		int32_t dy = Position::getDistanceY(getPosition(), attackedCreature->getPosition());

		if (dx > 10 || dy > 10) {
			setAttackedCreature(nullptr);
			Target = nullptr;
		}

		if (attackedCreature && getPosition().z != attackedCreature->getPosition().z) {
			setAttackedCreature(nullptr);
		}

		if (attackedCreature && attackedCreature->getTile()->hasFlag(TILESTATE_PROTECTIONZONE)) {
			setAttackedCreature(nullptr);
			Target = nullptr;
		}

		if (attackedCreature && attackedCreature->getTile()->getHouse()) {
			setAttackedCreature(nullptr);
			Target = nullptr;
		}

		if (attackedCreature && attackedCreature->isInvisible() && !canSeeInvisibility()) {
			setAttackedCreature(nullptr);
			Target = nullptr;
		}

		if (attackedCreature && attackedCreature->isRemoved() && attackedCreature->getHealth() <= 0) {
			setAttackedCreature(nullptr);
			Target = nullptr;
		}

		if (attackedCreature != master && mType->info.changeTargetChance > random(0, 99)) {
			setAttackedCreature(nullptr);
			Target = nullptr;
		}
	}

	if (State != STATE::PANIC && State != STATE::UNDERATTACK)
		State = STATE::IDLE;

	checkVoices();

	if (!attackedCreature) {
		bool Sleep = true;

		if (!isSummon()) {
			int32_t Goodness = INT32_MIN;
			int32_t TieBreaker = 0;
			int32_t Strategy = 0;
			int32_t r = random(0, 99);

			if (r < mType->info.strategyNearestEnemy) {
				Strategy = 0;
			}
			else {
				r -= mType->info.strategyNearestEnemy;
			}

			if (r < mType->info.strategyWeakestEnemy) {
				Strategy = 1;
			}
			else {
				r -= mType->info.strategyWeakestEnemy;
			}

			if (r < mType->info.strategyMostDamageEnemy) {
				Strategy = 2;
			}

			SpectatorVec spectators;
			g_game.map.getSpectators(spectators, getPosition(), true, false, 12, 12, 12, 12);
			spectators.erase(this); // Always delete self

			for (Creature* cr : spectators) {
				const Position& pos = cr->getPosition();
				const Position& myPos = getPosition();

				if (pos.z != myPos.z)
					continue;

				if (!isOpponent(cr))
					continue;

				Sleep = false;

				if (!isTarget(cr))
					continue;

				if (cr->getPlayer() && cr->getPlayer()->hasFlag(PlayerFlag_IgnoredByMonsters))
					continue;

				int32_t dx = Position::getDistanceX(myPos, pos);
				int32_t dy = Position::getDistanceY(myPos, pos);

				if (dx > 10 || dy > 10)
					continue;

				Sleep = false;

				if (!canSeeInvisibility() && cr->isInvisible())
					continue;

				if (cr->getTile()->hasFlag(TILESTATE_PROTECTIONZONE))
					continue;

				int32_t Priority = 0;

				switch (Strategy) {
				case 1:
					Priority = -cr->getHealth();
					break;
				case 2:
					Priority = getDamageDealtByAttacker(cr);
					break;
				case 3:
					Priority = random(0, 99);
					break;
				case 0:
					Priority = -(dy + dx);
					break;
				default:
					std::cout << "[Error - Monster::onIdleStimulus] Invalid strategy: " << Strategy << " for monster " << getName() << std::endl;
					break;
				}

				int32_t r = random(0, 99);
				if (Priority > Goodness || Priority == Goodness && r > TieBreaker) {
					setAttackedCreature(cr);
					Goodness = Priority;
					TieBreaker = r;
				}
			}
		}

		if (!attackedCreature) {
			if (Sleep) {
				if (State != STATE::UNDERATTACK && State != STATE::PANIC) {
					if (!isSummon()) {
						State = STATE::SLEEPING;
						setIdle(true);
						return;
					}

					setIdle(false);
					addWaitToDo(1000);
					startToDo();
					return;
				}
			}

			setIdle(false);

			if (State == STATE::PANIC)
				State = STATE::IDLE;
		}
	}

	if (State == STATE::UNDERATTACK)
		State = STATE::IDLE;

	doAttackSpells();
	doDefensiveSpells();
	spawnSummons();

	try
	{
		if (!attackedCreature)
			throw ReturnValue::RETURNVALUE_THEREISNOWAY;

		if (!isSummon() && isFleeing()) {
			Direction dir;
			if (getFlightStep(attackedCreature->getPosition(), dir)) {
				addWalkToDo(dir);
				startToDo();
				return;
			}

			throw ReturnValue::RETURNVALUE_TOOFARAWAY;
		}

		if (!attackedCreature && isSummon() || attackedCreature == master) {
			int32_t dx = Position::getDistanceX(getPosition(), master->getPosition());
			int32_t dy = Position::getDistanceY(getPosition(), master->getPosition());

			if (dx + dy > 1) {
				if (dx + dy == 2) {
					addWaitToDo(1000);
					startToDo();
					return;
				}

				if (dx + dy == 3)
					addWaitToDo(1000);

				std::vector<Direction> dirs;
				if (!getPathTo(master->getPosition(), dirs, 0, 1, true, true, 12)) {
					throw ReturnValue::RETURNVALUE_THEREISNOWAY;
				}

				addWalkToDo(dirs, 1);
				startToDo();
				return;
			}

			throw ReturnValue::RETURNVALUE_TOOFARAWAY;
		}

		if (attackedCreature != master) {
			if (mType->info.baseSkill > 0) {
				if (State != STATE::PANIC)
					State = STATE::ATTACKING;
			}

			if (static_cast<uint32_t>(State - 5) <= 1) {
				if (attackedCreature != Target) {
					attackedCreature->onAttacked();
					Target = attackedCreature;
				}

				chaseMode = false;
			}

			if (mType->info.targetDistance > 1 && g_game.canThrowObjectTo(getPosition(), attackedCreature->getPosition(), false)) {
				int32_t Distance = std::max<int32_t>(Position::getDistanceX(getPosition(), attackedCreature->getPosition()), Position::getDistanceY(attackedCreature->getPosition(), getPosition()));

				if (Distance <= 3) {
					Direction dir;
					if (getFlightStep(attackedCreature->getPosition(), dir))
						addWalkToDo(dir);
					else
						addWaitToDo(1000);
				}
				else if (Distance == 4) {
					int32_t x = getPosition().x;
					int32_t y = getPosition().y;
					int32_t r = rand() % 5;
					Direction dir = DIRECTION_NORTH;

					switch (r) {
					case 0:
						x--;
						dir = DIRECTION_WEST;
						break;
					case 1:
						x++;
						dir = DIRECTION_EAST;
						break;
					case 2:
						y--;
						dir = DIRECTION_NORTH;
						break;
					case 3:
						y++;
						dir = DIRECTION_SOUTH;
						break;
					default:
						break;
					}

					if (r <= 3 && canWalkTo(getPosition(), dir)) {
						int32_t dx = attackedCreature->getPosition().x - x;
						if (x - attackedCreature->getPosition().x > -1)
							dx = x - attackedCreature->getPosition().x;

						int32_t dy = attackedCreature->getPosition().y - y;
						if (y - attackedCreature->getPosition().y > -1)
							dy = y - attackedCreature->getPosition().y;

						if (std::max<int32_t>(dx, dy) == 4)
							addWalkToDo(dir);
					}

					addWaitToDo(1000);
				}
				else {
					std::vector<Direction> dirs;
					if (!getPathTo(attackedCreature->getPosition(), dirs, 0, 1, true, true, 8)) {
						throw ReturnValue::RETURNVALUE_THEREISNOWAY;
					}

					addWalkToDo(dirs, Distance - 4);
				}
			}
			else {
				chaseMode = true;

				if (Position::getDistanceX(getPosition(), attackedCreature->getPosition()) <= 1 && Position::getDistanceY(getPosition(), attackedCreature->getPosition()) <= 1) {
					int32_t r = rand() % 5;
					int32_t x = getPosition().x;
					int32_t y = getPosition().y;
					Direction dir = DIRECTION_NORTH;

					switch (r) {
					case 0:
						x--;
						dir = DIRECTION_WEST;
						break;
					case 1:
						x++;
						dir = DIRECTION_EAST;
						break;
					case 2:
						y--;
						dir = DIRECTION_NORTH;
						break;
					case 3:
						y++;
						dir = DIRECTION_SOUTH;
						break;
					default:
						break;
					}

					if (r <= 3 && canWalkTo(getPosition(), dir) && std::abs(x - attackedCreature->getPosition().x) <= 1 && std::abs(y - attackedCreature->getPosition().y) <= 1) {
						addWalkToDo(dir);
					}

					if (State == STATE::PANIC)
						State = STATE::ATTACKING;
				}
				else if (static_cast<uint32_t>(State - 5) > 1) {
					pathBlockCheck = true;

					std::vector<Direction> dirs;
					if (!getPathTo(attackedCreature->getPosition(), dirs, 0, 1, true, true, 8))
						throw ReturnValue::RETURNVALUE_THEREISNOWAY;

					addWalkToDo(dirs, 3);
				}
			}

			if (std::abs(State - 5) > 1) {
				addWaitToDo(1000);
			}
			else {
				updateLookDirection();

				if (mType->info.targetDistance == 1 && (Position::getDistanceX(getPosition(), attackedCreature->getPosition()) > mType->info.targetDistance ||
					Position::getDistanceY(getPosition(), attackedCreature->getPosition()) > mType->info.targetDistance) ||
					mType->info.targetDistance > 1 && !g_game.canThrowObjectTo(getPosition(), attackedCreature->getPosition(), false)) {
					pathBlockCheck = true;

					std::vector<Direction> dirs;
					if (!getPathTo(attackedCreature->getPosition(), dirs, 0, 1, true, true, 8))
						throw ReturnValue::RETURNVALUE_THEREISNOWAY;

					addWalkToDo(dirs, 3);
				}

				addAttackToDo();
				addWaitToDo(100);
			}

			startToDo();
			return;
		}
	}
	catch (ReturnValue& r)
	{
		pathBlockCheck = false;

		clearToDo();

		if (r != RETURNVALUE_TOOFARAWAY)
			setAttackedCreature(nullptr);

		addWaitToDo(100);
	}

	Direction dir;
	if (getRandomStep(getPosition(), dir)) {
		addWalkToDo(dir);
	}

	addWaitToDo(1000);
	startToDo();
}

void Monster::onThink(uint32_t interval)
{
	Creature::onThink(interval);

	if (mType->info.thinkEvent != -1) {
		// onThink(self, interval)
		LuaScriptInterface* scriptInterface = mType->info.scriptInterface;
		if (!scriptInterface->reserveScriptEnv()) {
			std::cout << "[Error - Monster::onThink] Call stack overflow" << std::endl;
			return;
		}

		ScriptEnvironment* env = scriptInterface->getScriptEnv();
		env->setScriptId(mType->info.thinkEvent, scriptInterface);

		lua_State* L = scriptInterface->getLuaState();
		scriptInterface->pushFunction(mType->info.thinkEvent);

		LuaScriptInterface::pushUserdata<Monster>(L, this);
		LuaScriptInterface::setMetatable(L, -1, "Monster");

		lua_pushnumber(L, interval);

		if (scriptInterface->callFunction(2)) {
			return;
		}
	}
}

void Monster::doAttacking()
{
	if (!attackedCreature || getHealth() <= 0 || isRemoved() || mType->info.baseSkill == 0 || !isSummon() && mType->info.runAwayHealth == mType->info.healthMax) {
		return;
	}

	if (attackedCreature == master && master) {
		return;
	}

	const Position& myPos = getPosition();
	const Position& targetPos = attackedCreature->getPosition();

	int64_t nextAttackTime = OTSYS_TIME() + 200;
	if (earliestMeleeAttack >= nextAttackTime) {
		nextAttackTime = earliestMeleeAttack;
	}
	earliestMeleeAttack = nextAttackTime;

	for (spellBlock_t& spellBlock : mType->info.attackSpells) {
		if (!spellBlock.isMelee) {
			continue;
		}

		if (attackedCreature->getZone() != ZONE_PROTECTION && Position::areInRange<1, 1, 0>(myPos, targetPos)) {
			spellBlock.minCombatValue = spellBlock.maxCombatValue = -Weapons::getMaxMeleeDamage(currentSkill, mType->info.baseAttack);
			minCombatValue = maxCombatValue = spellBlock.maxCombatValue;

			spellBlock.spell->castSpell(this, attackedCreature);

			addSkillPoint();

			nextAttackTime = OTSYS_TIME() + 2000;
			if (earliestMeleeAttack >= nextAttackTime) {
				nextAttackTime = earliestMeleeAttack;
			}
			earliestMeleeAttack = nextAttackTime;
		}

		break;
	}
}

bool Monster::pushItem(const Position& fromPos, Item* item)
{
	const Position& itemPos = item->getPosition();
	Cylinder* toCylinder = nullptr;
	Tile* tile;

	if (itemPos.y - 1 == fromPos.y) {
		tile = g_game.map.getTile(itemPos.x, itemPos.y + 1, itemPos.z);
		if (tile && tile->getGround() && !tile->hasFlag(TILESTATE_BLOCKSOLID) && tile->getCreatureCount() == 0) {
			toCylinder = tile;
		}
	}

	if (itemPos.y + 1 == fromPos.y) {
		tile = g_game.map.getTile(itemPos.x, itemPos.y - 1, itemPos.z);
		if (tile && tile->getGround() && !tile->hasFlag(TILESTATE_BLOCKSOLID) && tile->getCreatureCount() == 0) {
			toCylinder = tile;
		}
	}

	if (itemPos.x - 1 == fromPos.x) {
		tile = g_game.map.getTile(itemPos.x + 1, itemPos.y, itemPos.z);
		if (tile && tile->getGround() && !tile->hasFlag(TILESTATE_BLOCKSOLID) && tile->getCreatureCount() == 0) {
			toCylinder = tile;
		}
	}

	if (itemPos.x + 1 == fromPos.x) {
		tile = g_game.map.getTile(itemPos.x - 1, itemPos.y, itemPos.z);
		if (tile && tile->getGround() && !tile->hasFlag(TILESTATE_BLOCKSOLID) && tile->getCreatureCount() == 0) {
			toCylinder = tile;
		}
	}

	if (!toCylinder) {
		tile = g_game.map.getTile(itemPos.x, itemPos.y - 1, itemPos.z);
		if (itemPos.y - 1 != fromPos.y && tile && tile->getGround() && !tile->hasFlag(TILESTATE_BLOCKSOLID) && tile->getCreatureCount() == 0) {
			toCylinder = tile;
		} else {
			tile = g_game.map.getTile(itemPos.x, itemPos.y + 1, itemPos.z);
			if (fromPos.y - 1 != itemPos.y && tile && tile->getGround() && !tile->hasFlag(TILESTATE_BLOCKSOLID) && tile->getCreatureCount() == 0) {
				toCylinder = tile;
			} else {
				tile = g_game.map.getTile(itemPos.x - 1, itemPos.y, itemPos.z);
				if (fromPos.x + 1 != itemPos.x && tile && tile->getGround() && !tile->hasFlag(TILESTATE_BLOCKSOLID) && tile->getCreatureCount() == 0) {
					toCylinder = tile;
				} else {
					tile = g_game.map.getTile(itemPos.x + 1, itemPos.y, itemPos.z);
					if (fromPos.x - 1 != itemPos.x && tile && tile->getGround() && !tile->hasFlag(TILESTATE_BLOCKSOLID) && tile->getCreatureCount() == 0) {
						toCylinder = tile;
					}
				}
			}
		}
	}

	if (toCylinder && g_game.internalMoveItem(item->getParent(), toCylinder, INDEX_WHEREEVER, item, item->getItemCount(), nullptr) == RETURNVALUE_NOERROR) {
		return true;
	}

	return false;
}

void Monster::pushItems(const Position& fromPos, Tile* fromTile)
{
	//We can not use iterators here since we can push the item to another tile
	//which will invalidate the iterator.
	//start from the end to minimize the amount of traffic
	if (const TileItemVector* items = fromTile->getItemList()) {
		uint32_t moveCount = 0;
		uint32_t removeCount = 0;

		const int32_t downItemSize = fromTile->getDownItemCount();
		for (int32_t i = downItemSize; --i >= 0;) {
			Item* item = items->at(i);
			if (item && item->hasProperty(CONST_PROP_MOVEABLE) && (item->hasProperty(CONST_PROP_BLOCKPATH)
			        || item->hasProperty(CONST_PROP_BLOCKSOLID))) {
				if (item->getActionId() >= 1000 && item->getActionId() <= 2000) {
					continue;
				}

				if (moveCount < 20 && Monster::pushItem(fromPos, item)) {
					++moveCount;
				} else if (g_game.internalRemoveItem(item) == RETURNVALUE_NOERROR) {
					++removeCount;
				}
			}
		}

		if (removeCount > 0) {
			g_game.addMagicEffect(fromTile->getPosition(), CONST_ME_BLOCKHIT);
		}
	}
}

bool Monster::pushCreature(const Position& fromPos, Creature* creature)
{
	int32_t cx = creature->getPosition().x;
	int32_t cy = creature->getPosition().y;

	Monster* monster = creature->getMonster();
	if (!monster) {
		return false;
	}

	if (cy - 1 != fromPos.y) {
		Tile* toTile = g_game.map.getTile(cx, cy - 1, creature->getPosition().z);
		if (!monster->canPushItems() && toTile && !toTile->hasProperty(ITEMPROPERTY::CONST_PROP_BLOCKPATH) &&
			toTile->getCreatureCount() == 0) {
			if (g_game.internalMoveCreature(creature, DIRECTION_NORTH) == RETURNVALUE_NOERROR) {
				return true;
			}
		}
		else if (monster->canPushItems()) {
			if (g_game.internalMoveCreature(creature, DIRECTION_NORTH) == RETURNVALUE_NOERROR) {
				return true;
			}
		}
	}

	if (cy + 1 != fromPos.y) {
		Tile* toTile = g_game.map.getTile(cx, cy + 1, creature->getPosition().z);
		if (!monster->canPushItems() && toTile && !toTile->hasProperty(ITEMPROPERTY::CONST_PROP_BLOCKPATH) &&
			toTile->getCreatureCount() == 0) {
			if (g_game.internalMoveCreature(creature, DIRECTION_SOUTH) == RETURNVALUE_NOERROR) {
				return true;
			}
		}
		else if (monster->canPushItems()) {
			if (g_game.internalMoveCreature(creature, DIRECTION_SOUTH) == RETURNVALUE_NOERROR) {
				return true;
			}
		}
	}

	if (cx - 1 != fromPos.x) {
		Tile* toTile = g_game.map.getTile(cx - 1, cy, creature->getPosition().z);
		if (!monster->canPushItems() && toTile && !toTile->hasProperty(ITEMPROPERTY::CONST_PROP_BLOCKPATH) &&
			toTile->getCreatureCount() == 0) {
			if (g_game.internalMoveCreature(creature, DIRECTION_WEST) == RETURNVALUE_NOERROR) {
				return true;
			}
		}
		else if (monster->canPushItems()) {
			if (g_game.internalMoveCreature(creature, DIRECTION_WEST) == RETURNVALUE_NOERROR) {
				return true;
			}
		}
	}

	if (cx + 1 != fromPos.x) {
		Tile* toTile = g_game.map.getTile(cx + 1, cy, creature->getPosition().z);
		if (!monster->canPushItems() && toTile && !toTile->hasProperty(ITEMPROPERTY::CONST_PROP_BLOCKPATH) &&
			toTile->getCreatureCount() == 0) {
			if (g_game.internalMoveCreature(creature, DIRECTION_EAST) == RETURNVALUE_NOERROR) {
				return true;
			}
		} 
		else if (monster->canPushItems()) {
			if (g_game.internalMoveCreature(creature, DIRECTION_EAST) == RETURNVALUE_NOERROR) {
				return true;
			}
		}
	}

	return false;
}

bool Monster::pushCreatures(const Position& fromPos, Tile* fromTile, Creature* pushingCreature)
{
	//We can not use iterators here since we can push a creature to another tile
	//which will invalidate the iterator.
	if (const CreatureVector* creatures = fromTile->getCreatures()) {
		const Monster* lastPushedMonster = nullptr;

		for (size_t i = 0; i < creatures->size();) {
			Monster* monster = creatures->at(i)->getMonster();
			if (pushingCreature == monster) {
				++i;
				continue;
			}

			if (const Monster* pushingMonster = pushingCreature->getMonster()) {
				if (pushingMonster->canPushCreatures() && pushingMonster->isPushable()) {
					if (monster->isPushable() && monster->canPushCreatures()) {
						i++;
						continue;
					}
				}
			}

			// we only allow kicking 1 creature at a time.
			if (monster && monster->isPushable()) {
				if (monster->getHealth() > 0) {
					if (monster != lastPushedMonster && Monster::pushCreature(fromPos, monster)) {
						lastPushedMonster = monster;
						return true;
					}

					g_game.addMagicEffect(monster->getPosition(), CONST_ME_BLOCKHIT);

					if (pushingCreature) {
						monster->addDamagePoints(pushingCreature, monster->getHealth());
					}
					monster->changeHealth(-monster->getHealth());
				}

				return false;
			}

			++i;
		}
	}

	return true;
}

bool Monster::getRandomStep(const Position& creaturePos, Direction& resultDir) const
{
	static std::vector<Direction> dirList{
			DIRECTION_NORTH,
		DIRECTION_WEST, DIRECTION_EAST,
			DIRECTION_SOUTH
	};
	std::shuffle(dirList.begin(), dirList.end(), getRandomGenerator());

	for (const Direction dir : dirList) {
		if (canWalkTo(creaturePos, dir)) {
			resultDir = dir;
			return true;
		}
	}
	return false;
}

bool Monster::getFlightStep(const Position& targetPos, Direction& resultDir) const
{
	if (getBaseSpeed() <= 0)
		return false;

	const Position& creaturePos = getPosition();

	int32_t offsetx = Position::getOffsetX(creaturePos, targetPos);
	int32_t offsety = Position::getOffsetY(creaturePos, targetPos);
	int32_t reverseoffsety = Position::getOffsetY(targetPos, creaturePos);

	if (offsety > -1) {
		reverseoffsety = offsety;
	}

	Direction firstStep = DIRECTION_NONE;

	if (offsetx > reverseoffsety) {
		firstStep = DIRECTION_EAST; // 1
	}

	int32_t helper = offsetx;
	if (offsetx <= -1) {
		helper = -offsetx;
	}

	if (reverseoffsety > helper) {
		firstStep = DIRECTION_NORTH; // 3
	}

	helper = Position::getOffsetY(targetPos, creaturePos);
	if (offsety > -1) {
		helper = offsety;
	}

	if (-offsetx > helper) {
		firstStep = DIRECTION_WEST; // 5
	}

	helper = offsetx;
	if (offsetx <= -1) {
		helper = -offsetx;
	}

	if (offsety > helper) {
		firstStep = DIRECTION_SOUTH; // 7
	}

	///////////////////

	if (canWalkTo(creaturePos, firstStep)) {
		resultDir = firstStep;
		return true;
	}

	// list of possible directions (not-checked)
	std::vector<Direction> directions;
	std::vector<Direction> diagonalDirections;

	if (offsetx >= 0) {
		directions.push_back(DIRECTION_EAST);
	}

	if (offsety <= 0) {
		directions.push_back(DIRECTION_NORTH);
	}

	if (offsetx <= 0) {
		directions.push_back(DIRECTION_WEST);
	}

	if (offsety >= 0) {
		directions.push_back(DIRECTION_SOUTH);
	}

	std::shuffle(std::begin(directions), std::end(directions), getRandomGenerator());

	if (offsetx >= 1 && offsety <= -1 || offsety < 0 && offsetx > 0) {
		diagonalDirections.push_back(DIRECTION_NORTHEAST);
	}

	if (offsetx <= -1 && offsety <= -1 || offsety < 0 && offsetx < 0) {
		diagonalDirections.push_back(DIRECTION_NORTHWEST);
	}

	if (offsetx >= 1 && offsety >= 1 || offsety > 0 && offsetx > 0) {
		diagonalDirections.push_back(DIRECTION_SOUTHEAST);
	}

	if (offsetx <= -1 && offsety >= 1 || offsety > 0 && offsetx < 0) {
		diagonalDirections.push_back(DIRECTION_SOUTHWEST);
	}

	if (diagonalDirections.empty()) {
		if (creaturePos.x - targetPos.x >= creaturePos.y - targetPos.y) {
			directions.push_back(DIRECTION_NORTHEAST);
		}

		if (creaturePos.x - targetPos.x <= targetPos.y - creaturePos.y) {
			directions.push_back(DIRECTION_NORTHWEST);
		}

		if (creaturePos.x - targetPos.x <= creaturePos.y - targetPos.y) {
			directions.push_back(DIRECTION_SOUTHWEST);
		}

		if (creaturePos.x - targetPos.x >= -(creaturePos.y - targetPos.y)) {
			directions.push_back(DIRECTION_SOUTHEAST);
		}
	}

	std::shuffle(std::begin(diagonalDirections), std::end(diagonalDirections), getRandomGenerator());

	for (const Direction& dir : directions) {
		if (canWalkTo(creaturePos, dir)) {
			resultDir = dir;
			return true;
		}
	}

	for (const Direction& dir : diagonalDirections) {
		if (canWalkTo(creaturePos, dir)) {
			resultDir = dir;
			return true;
		}
	}

	return false;
}

bool Monster::canWalkTo(Position pos, Direction dir) const
{
	pos = getNextPosition(dir, pos);
	if (isInSpawnRange(getPosition()) && !isInSpawnRange(pos)) {
		return false;
	}

	const Tile* tile = g_game.map.getTile(pos);
	if (!tile) {
		return false;
	}

	uint32_t flags = FLAG_PATHFINDING;
	if (isPathBlockingChecking()) {
		flags |= FLAG_IGNOREBLOCKCREATURE;
	}

	if (tile->queryAdd(0, *this, 1, flags) != RETURNVALUE_NOERROR) {
		return false;
	}

	const Creature* topCreature = tile->getTopVisibleCreature(this);
	if (topCreature && (!canPushCreatures() || !topCreature->isPushable())) {
		return false;
	}

	return true;
}

void Monster::death(Creature*)
{
	setAttackedCreature(nullptr);

	onIdleStatus();
}

Item* Monster::getCorpse(Creature* lastHitCreature, Creature* mostDamageCreature)
{
	Item* corpse = Creature::getCorpse(lastHitCreature, mostDamageCreature);
	if (corpse) {
		corpse->specialCorpseDrop = true;
		if (mostDamageCreature) {
			if (mostDamageCreature->getPlayer()) {
				corpse->setCorpseOwner(mostDamageCreature->getID());
			} else {
				const Creature* mostDamageCreatureMaster = mostDamageCreature->getMaster();
				if (mostDamageCreatureMaster && mostDamageCreatureMaster->getPlayer()) {
					corpse->setCorpseOwner(mostDamageCreatureMaster->getID());
				}
			}
		}
	}
	return corpse;
}

bool Monster::isInSpawnRange(const Position& pos) const
{
	if (!spawn) {
		return true;
	}

	if (!g_config.getBoolean(ConfigManager::ALLOW_MONSTER_OVERSPAWN) && !isFleeing()) {
		return true;
	}

	if (!Spawns::isInZone(masterPos, spawn->getRadius(), pos)) {
		return false;
	}

	return true;
}

bool Monster::getCombatValues(int32_t& min, int32_t& max)
{
	if (minCombatValue == 0 && maxCombatValue == 0) {
		return false;
	}

	min = minCombatValue;
	max = maxCombatValue;
	return true;
}

void Monster::doAttackSpells()
{
	if (!attackedCreature || attackedCreature == master && master) {
		return;
	}

	if (attackedCreature->getPosition().z != getPosition().z) {
		return;
	}

	for (const spellBlock_t& spellBlock : mType->info.attackSpells) {
		if (spellBlock.isMelee) {
			continue;
		}

		if (!(rand() % spellBlock.delay) && (isSummon() || !isFleeing() || random(1, 3) == 1)) {
			if (spellBlock.updateLook) {
				updateLookDirection();
			}

			if (spellBlock.range != 0) {
				if (!attackedCreature) {
					continue;
				}

				const Position& myPos = getPosition();
				const Position& targetPos = attackedCreature->getPosition();
				const int32_t targetDistance = std::max<int32_t>(Position::getDistanceX(myPos, targetPos), Position::getDistanceY(myPos, targetPos));
				
				if (!g_game.canThrowObjectTo(myPos, targetPos, false) || targetDistance > static_cast<int32_t>(spellBlock.range)) {
					continue;
				}
			}

			minCombatValue = spellBlock.minCombatValue;
			maxCombatValue = spellBlock.maxCombatValue;

			if (attackedCreature) {
				spellBlock.spell->castSpell(this, attackedCreature);
			} else {
				spellBlock.spell->castSpell(this, this);
			}
		}
	}
}

void Monster::doDefensiveSpells()
{
	for (const spellBlock_t& spellBlock : mType->info.defenseSpells) {
		if (!(rand() % spellBlock.delay) && (isSummon() || !isFleeing() || random(1, 3) == 1)) {
			if (spellBlock.updateLook) {
				updateLookDirection();
			}

			minCombatValue = spellBlock.minCombatValue;
			maxCombatValue = spellBlock.maxCombatValue;
			spellBlock.spell->castSpell(this, this);
		}
	}
}

void Monster::spawnSummons()
{
	if (!attackedCreature || attackedCreature->getPosition().z != getPosition().z) {
		return;
	}

	if (!isSummon() && summons.size() < mType->info.maxSummons) {
		for (const summonBlock_t& summonBlock : mType->info.summons) {
			if (summons.size() >= mType->info.maxSummons) {
				continue;
			}

			uint32_t summonCount = 0;
			for (const Creature* summon : summons) {
				if (summon->getName() == summonBlock.name) {
					++summonCount;
				}
			}

			if (summonCount >= summonBlock.max) {
				continue;
			}

			if (!uniform_random(0, summonBlock.delay) && (isSummon() || !isFleeing() || uniform_random(1, 3) == 1)) {
				if (Monster* summon = Monster::createMonster(summonBlock.name)) {
					Position pos = getPosition();
					g_game.searchSummonField(pos.x, pos.y, pos.z, 2);
					if (g_game.placeCreature(summon, pos, summonBlock.force)) {
						summon->setDropLoot(false);
						summon->setSkillLoss(false);
						summon->setMaster(this);
						g_game.addMagicEffect(getPosition(), CONST_ME_MAGIC_BLUE);
						g_game.addMagicEffect(summon->getPosition(), CONST_ME_TELEPORT);
					}
					else {
						delete summon;
					}
				}
			}
		}
	}
}

void Monster::checkVoices()
{
	if (!mType->info.voiceVector.empty()) {
		int32_t r = rand();
		if (r == 50 * (r / 50)) {
			r = rand();
			const voiceBlock_t& voiceBlock = mType->info.voiceVector[random(0, static_cast<int32_t>(mType->info.voiceVector.size() - 1))];
			g_game.internalCreatureSay(this, voiceBlock.yellText ? TALKTYPE_MONSTER_YELL : TALKTYPE_MONSTER_SAY, voiceBlock.text, false);
		}
	}
}

void Monster::updateLookDirection()
{
	Direction newDir = DIRECTION_NONE;

	if (attackedCreature) {
		const Position& pos = getPosition();
		const Position& attackedCreaturePos = attackedCreature->getPosition();

		const int32_t offsetx = Position::getOffsetX(attackedCreaturePos, pos);
		int32_t offsetxr = Position::getOffsetX(pos, attackedCreaturePos);
		const int32_t offsety = Position::getOffsetY(attackedCreaturePos, pos);

		if (offsetx > -1) {
			offsetxr = offsetx;
		}

		int32_t offsetyr = Position::getOffsetY(pos, attackedCreaturePos);

		if (offsety > -1) {
			offsetyr = offsety;
		}

		int32_t value;
		if (offsetxr >= offsetyr) {
			value = 2 * (static_cast<uint32_t>(offsetx) >> 31) + 1;
		} else {
			value = 2 * (offsety >= 0);
		}

		newDir = static_cast<Direction>(value);
	}

	if (newDir != DIRECTION_NONE) {
		g_game.internalCreatureTurn(this, newDir);
	}
}

void Monster::dropLoot(Container* corpse, Creature*)
{
	if (corpse && lootDrop) {
		if (g_config.getBoolean(ConfigManager::MONSTERS_SPAWN_WITH_LOOT)) {
			for (int32_t slot = CONST_SLOT_FIRST; slot <= CONST_SLOT_LAST; slot++) {
				Item* inventoryItem = inventory[slot];
				if (!inventoryItem) {
					continue;
				}

				if (corpse->queryAdd(INDEX_WHEREEVER, *inventoryItem, inventoryItem->getItemCount(), 0) == RETURNVALUE_NOERROR) {
					corpse->internalAddThing(inventoryItem);
				} else {
					inventoryItem->decrementReferenceCounter();
				}

				// remove item from the inventory
				inventory[slot] = nullptr;
			}
		}

		g_events->eventMonsterOnDropLoot(this, corpse);
	}
}

void Monster::drainHealth(Creature* attacker, int32_t damage)
{
	Creature::drainHealth(attacker, damage);

	if (damage > 0 && attacker) {
		if (State >= STATE::IDLE) {
			if (State == STATE::IDLE) {
				State = STATE::UNDERATTACK;
			} 

			if (!attackedCreature) {
				State = STATE::PANIC;
			}
		} else {
			State = static_cast<STATE>(4 * (attackedCreature == nullptr) + 2);
			addYieldToDo();
		}
	}

	if (isInvisible()) {
		removeCondition(CONDITION_INVISIBLE);
	}
}

void Monster::changeHealth(int32_t healthChange, bool sendHealthChange/* = true*/)
{
	//In case a player with ignore flag set attacks the monster
	setIdle(false);
	Creature::changeHealth(healthChange, sendHealthChange);
}

LightInfo Monster::getCreatureLight() const
{
	if (internalLight.level != 0) {
		return internalLight;
	}

	LightInfo light = Creature::getCreatureLight();
	for (int32_t slot = CONST_SLOT_FIRST; slot <= CONST_SLOT_LAST; slot++) {
		const Item* inventoryItem = inventory[slot];
		if (!inventoryItem) {
			continue;
		}

		const LightInfo itemLight = inventoryItem->getLightInfo();
		if (itemLight.level > light.level) {
			light = itemLight;
		}
	}

	return light;
}

bool Monster::challengeCreature(Creature* creature, bool force/* = false*/)
{
	if (isSummon()) {
		return false;
	}

	if (!mType->info.isChallengeable && !force) {
		return false;
	}

	return selectTarget(creature);
}

slots_t getItemSlotType(const ItemType& it)
{
	slots_t slot = CONST_SLOT_RIGHT;
	if (it.weaponType != WeaponType_t::WEAPON_SHIELD) {
		const int32_t slotPosition = it.slotPosition;

		if (slotPosition & SLOTP_HEAD) {
			slot = CONST_SLOT_HEAD;
		} else if (slotPosition & SLOTP_NECKLACE) {
			slot = CONST_SLOT_NECKLACE;
		} else if (slotPosition & SLOTP_ARMOR) {
			slot = CONST_SLOT_ARMOR;
		} else if (slotPosition & SLOTP_LEGS) {
			slot = CONST_SLOT_LEGS;
		} else if (slotPosition & SLOTP_FEET) {
			slot = CONST_SLOT_FEET;
		} else if (slotPosition & SLOTP_RING) {
			slot = CONST_SLOT_RING;
		} else if (slotPosition & SLOTP_AMMO) {
			slot = CONST_SLOT_AMMO;
		} else if (slotPosition & SLOTP_TWO_HAND || slotPosition & SLOTP_LEFT) {
			slot = CONST_SLOT_LEFT;
		}
	}

	return slot;
}
int32_t Monster::getArmor() const 
{
	int32_t armor = mType->info.armor;

	// inventory armor increase
	if (g_config.getBoolean(ConfigManager::MONSTERS_SPAWN_WITH_LOOT)) {
		for (int32_t slot = CONST_SLOT_FIRST; slot <= CONST_SLOT_LAST; slot++) {
			const Item* inventoryItem = inventory[slot];
			if (!inventoryItem) {
				continue;
			}

			if (slot == getItemSlotType(Item::items.getItemType(inventoryItem->getID()))) {
				armor += inventoryItem->getArmor();
			}
		}
	}

	if (g_config.getBoolean(ConfigManager::USE_CLASSIC_COMBAT_FORMULAS)) {
		if (armor > 1) {
			armor = rand() % (armor >> 1) + (armor >> 1);
		}
	}

	return armor;
}

int32_t Monster::getDefense() const
{
	int32_t totalDefense = mType->info.defense;

	if (g_config.getBoolean(ConfigManager::USE_CLASSIC_COMBAT_FORMULAS)) {
		fightMode_t newFightMode = FIGHTMODE_BALANCED;
		if (!attackedCreature && OTSYS_TIME() >= earliestMeleeAttack) {
			newFightMode = FIGHTMODE_DEFENSE;
		}

		if (newFightMode == FIGHTMODE_DEFENSE) {
			totalDefense += 8 * totalDefense / 10;
		} // monsters are never in full attack mode

		const int32_t formula = (5 * (currentSkill) + 50) * totalDefense;
		const int32_t rnd = rand() % 100;
		totalDefense = formula * ((rand() % 100 + rnd) / 2) / 10000;
	}

	return totalDefense;
}

bool Monster::canPushItems() const
{
	if (const Monster* master = this->master ? this->master->getMonster() : nullptr) {
		return master->mType->info.canPushItems;
	}

	return mType->info.canPushItems;
}
