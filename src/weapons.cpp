// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "combat.h"
#include "configmanager.h"
#include "game.h"
#include "weapons.h"

extern Game g_game;
extern Vocations g_vocations;
extern ConfigManager g_config;
extern Weapons* g_weapons;

Weapons::Weapons() { scriptInterface.initState(); }

Weapons::~Weapons() { clear(); }

const Weapon* Weapons::getWeapon(const Item* item) const
{
	if (!item) {
		return nullptr;
	}

	auto it = weapons.find(item->getID());
	if (it == weapons.end()) {
		return nullptr;
	}
	return it->second;
}

void Weapons::clear()
{
	weapons.clear();

	getScriptInterface().reInitState();
}

LuaScriptInterface& Weapons::getScriptInterface() { return scriptInterface; }

std::string Weapons::getScriptBaseName() const { return "weapons"; }

void Weapons::loadDefaults()
{
	for (size_t i = 100, size = Item::items.size(); i < size; ++i) {
		const ItemType& it = Item::items.getItemType(i);
		if (it.id == 0 || weapons.find(i) != weapons.end()) {
			continue;
		}

		switch (it.weaponType) {
			case WEAPON_AXE:
			case WEAPON_SWORD:
			case WEAPON_CLUB: {
				WeaponMelee* weapon = new WeaponMelee(&scriptInterface);
				weapon->configureWeapon(it);
				weapons[i] = weapon;
				break;
			}

			case WEAPON_AMMO:
			case WEAPON_DISTANCE: {
				if (it.weaponType == WEAPON_DISTANCE && it.ammoType != AMMO_NONE) {
					continue;
				}

				WeaponDistance* weapon = new WeaponDistance(&scriptInterface);
				weapon->configureWeapon(it);
				weapons[i] = weapon;
				break;
			}

			default:
				break;
		}
	}
}

bool Weapons::registerLuaEvent(Weapon* weapon)
{
	weapons[weapon->getID()] = weapon;
	return true;
}

// monsters
int32_t Weapons::getMaxMeleeDamage(int32_t attackSkill, int32_t attackValue)
{
	if (g_config.getBoolean(ConfigManager::USE_CLASSIC_COMBAT_FORMULAS)) {
		int32_t maxWeaponDamage = attackValue;
		int32_t formula = (5 * (attackSkill) + 50) * maxWeaponDamage;
		int32_t rnd = rand() % 100;
		return formula * ((rand() % 100 + rnd) / 2) / 10000;
	}

	return static_cast<int32_t>(std::ceil((attackSkill * (attackValue * 0.05)) + (attackValue * 0.5)));
}

// players
int32_t Weapons::getMaxWeaponDamage(uint32_t level, int32_t attackSkill, int32_t attackValue, float attackFactor)
{
	if (g_config.getBoolean(ConfigManager::USE_CLASSIC_COMBAT_FORMULAS)) {
		int32_t maxWeaponDamage = attackValue;
		if (attackFactor == 1.0f) { // full attack factor
			maxWeaponDamage += 2 * maxWeaponDamage / 10;
		} else if (attackFactor == 2.0f) { // full defense attack factor
			maxWeaponDamage -= 4 * maxWeaponDamage / 10;
		}

		int32_t formula = (5 * (attackSkill) + 50) * maxWeaponDamage;

		int32_t rnd = rand() % 100;
		maxWeaponDamage = formula * ((rand() % 100 + rnd) / 2) / 10000;
		return maxWeaponDamage;
	}

	return static_cast<int32_t>(
	    std::round((level / 5) + (((((attackSkill / 4.) + 1) * (attackValue / 3.)) * 1.03) / attackFactor)));
}

void Weapon::configureWeapon(const ItemType& it) { id = it.id; }

std::string Weapon::getScriptEventName() const { return "onUseWeapon"; }

int32_t Weapon::playerWeaponCheck(Player* player, Creature* target, uint8_t shootRange) const
{
	const Position& playerPos = player->getPosition();
	const Position& targetPos = target->getPosition();
	if (playerPos.z != targetPos.z) {
		return 0;
	}

	if (std::max<uint32_t>(Position::getDistanceX(playerPos, targetPos), Position::getDistanceY(playerPos, targetPos)) >
	    shootRange) {
		return 0;
	}

	if (!player->hasFlag(PlayerFlag_IgnoreWeaponCheck)) {
		if (!enabled) {
			return 0;
		}

		if (player->getMana() < getManaCost(player)) {
			return 0;
		}

		if (player->getHealth() < getHealthCost(player)) {
			return 0;
		}

		if (player->getSoul() < soul) {
			return 0;
		}

		if (isPremium() && !player->isPremium()) {
			return 0;
		}

		if (!vocWeaponMap.empty()) {
			if (vocWeaponMap.find(player->getVocationId()) == vocWeaponMap.end()) {
				return 0;
			}
		}

		int32_t damageModifier = 100;
		if (player->getLevel() < getReqLevel()) {
			damageModifier = (isWieldedUnproperly() ? damageModifier / 2 : 0);
		}

		if (player->getMagicLevel() < getReqMagLv()) {
			damageModifier = (isWieldedUnproperly() ? damageModifier / 2 : 0);
		}
		return damageModifier;
	}

	return 100;
}

bool Weapon::useWeapon(Player* player, Item* item, Creature* target) const
{
	int32_t damageModifier = playerWeaponCheck(player, target, item->getShootRange());
	if (damageModifier == 0) {
		return false;
	}

	internalUseWeapon(player, item, target, damageModifier);
	return true;
}

bool Weapon::useFist(Player* player, Creature* target)
{
	if (!Position::areInRange<1, 1>(player->getPosition(), target->getPosition())) {
		return false;
	}

	float attackFactor = player->getAttackFactor();
	int32_t attackSkill = player->getSkillLevel(SKILL_FIST);
	int32_t attackValue = 7;

	int32_t maxDamage = Weapons::getMaxWeaponDamage(player->getLevel(), attackSkill, attackValue, attackFactor);

	CombatParams params;
	params.combatType = COMBAT_PHYSICALDAMAGE;
	params.blockedByArmor = true;
	params.blockedByShield = true;

	CombatDamage damage;
	damage.origin = ORIGIN_MELEE;
	damage.type = params.combatType;

	if (g_config.getBoolean(ConfigManager::USE_CLASSIC_COMBAT_FORMULAS)) {
		damage.value = -maxDamage;
	} else {
		damage.value = -random(0, maxDamage);
	}

	Combat::doTargetCombat(player, target, damage, params);
	if (!player->hasFlag(PlayerFlag_NotGainSkill) && player->getAddAttackSkill()) {
		if (player->getBloodHitCount() > 0) {
			player->decrementBloodHitCount(1);
			player->addSkillAdvance(SKILL_FIST, 1);
		}
	}

	return true;
}

void Weapon::internalUseWeapon(Player* player, Item* item, Creature* target, int32_t damageModifier) const
{
	if (scripted) {
		LuaVariant var;
		var.type = VARIANT_NUMBER;
		var.number = target->getID();
		executeUseWeapon(player, var);
	} else {
		CombatDamage damage;
		WeaponType_t weaponType = item->getWeaponType();
		if (weaponType == WEAPON_AMMO || weaponType == WEAPON_DISTANCE) {
			damage.origin = ORIGIN_RANGED;
		} else {
			damage.origin = ORIGIN_MELEE;
		}
		damage.type = params.combatType;
		damage.value = (getWeaponDamage(player, target, item) * damageModifier) / 100;
		Combat::doTargetCombat(player, target, damage, params);
	}

	onUsedWeapon(player, item, target->getTile());
}

void Weapon::internalUseWeapon(Player* player, Item* item, Tile* tile, bool hit) const
{
	if (scripted) {
		LuaVariant var;
		var.type = VARIANT_TARGETPOSITION;
		var.pos = tile->getPosition();
		executeUseWeapon(player, var, hit);

		if (!hit) {
			g_game.addMagicEffect(tile->getPosition(), CONST_ME_POFF);
		}
	} else {
		Combat::postCombatEffects(player, tile->getPosition(), params);
		g_game.addMagicEffect(tile->getPosition(), CONST_ME_POFF);
	}

	onUsedWeapon(player, item, tile);
}

void Weapon::onUsedWeapon(Player* player, Item* item, Tile* destTile) const
{
	if (!player->hasFlag(PlayerFlag_NotGainSkill)) {
		skills_t skillType;
		uint32_t skillPoint;
		if (getSkillType(player, item, skillType, skillPoint)) {
			skillPoint = std::min<uint32_t>(player->getBloodHitCount(), skillPoint);
			player->decrementBloodHitCount(skillPoint);
			player->addSkillAdvance(skillType, skillPoint);
		}
	}

	uint32_t manaCost = getManaCost(player);
	if (manaCost != 0 && !g_config.getBoolean(ConfigManager::UNLIMITED_PLAYER_MP)) {
		player->addManaSpent(manaCost);
		player->changeMana(-static_cast<int32_t>(manaCost));
	}

	uint32_t healthCost = getHealthCost(player);
	if (healthCost != 0) {
		player->changeHealth(-static_cast<int32_t>(healthCost));
	}

	if (!player->hasFlag(PlayerFlag_HasInfiniteSoul) && soul > 0) {
		player->changeSoul(-static_cast<int32_t>(soul));
	}

	if (breakChance != 0 && uniform_random(1, 100) <= breakChance) {
		Weapon::decrementItemCount(item);
		return;
	}

	switch (action) {
		case WEAPONACTION_REMOVECOUNT:
			if (g_config.getBoolean(ConfigManager::REMOVE_WEAPON_AMMO)) {
				Weapon::decrementItemCount(item);
			}
			break;

		case WEAPONACTION_REMOVECHARGE: {
			uint16_t charges = item->getCharges();
			if (charges != 0 && g_config.getBoolean(ConfigManager::REMOVE_WEAPON_CHARGES)) {
				g_game.transformItem(item, item->getID(), charges - 1);
			}
			break;
		}

		case WEAPONACTION_MOVE:
			g_game.internalMoveItem(item->getParent(), destTile, INDEX_WHEREEVER, item, 1, nullptr, FLAG_NOLIMIT);
			break;

		default:
			break;
	}
}

uint32_t Weapon::getManaCost(const Player* player) const
{
	if (mana != 0) {
		return mana;
	}

	if (manaPercent == 0) {
		return 0;
	}

	return (player->getMaxMana() * manaPercent) / 100;
}

int32_t Weapon::getHealthCost(const Player* player) const
{
	if (health != 0) {
		return health;
	}

	if (healthPercent == 0) {
		return 0;
	}

	return (player->getMaxHealth() * healthPercent) / 100;
}

bool Weapon::executeUseWeapon(Player* player, const LuaVariant& var, bool hit) const
{
	// onUseWeapon(player, var, hit)
	if (!scriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Weapon::executeUseWeapon] Call stack overflow" << std::endl;
		return false;
	}

	ScriptEnvironment* env = scriptInterface->getScriptEnv();
	env->setScriptId(scriptId, scriptInterface);

	lua_State* L = scriptInterface->getLuaState();

	scriptInterface->pushFunction(scriptId);
	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");
	scriptInterface->pushVariant(L, var);
	scriptInterface->pushBoolean(L, hit);

	return scriptInterface->callFunction(3);
}

void Weapon::decrementItemCount(Item* item)
{
	uint16_t count = item->getItemCount();
	if (count > 1) {
		g_game.transformItem(item, item->getID(), count - 1);
	} else {
		g_game.internalRemoveItem(item);
	}
}

WeaponMelee::WeaponMelee(LuaScriptInterface* interface) : Weapon(interface)
{
	params.blockedByArmor = true;
	params.blockedByShield = true;
	params.combatType = COMBAT_PHYSICALDAMAGE;
}

void WeaponMelee::configureWeapon(const ItemType& it)
{
	if (it.abilities) {
		elementType = it.abilities->elementType;
		elementDamage = it.abilities->elementDamage;
		params.aggressive = true;
		params.useCharges = true;
	} else {
		elementType = COMBAT_NONE;
		elementDamage = 0;
	}
	Weapon::configureWeapon(it);
}

bool WeaponMelee::useWeapon(Player* player, Item* item, Creature* target) const
{
	int32_t damageModifier = playerWeaponCheck(player, target, item->getShootRange());
	if (damageModifier == 0) {
		return false;
	}

	internalUseWeapon(player, item, target, damageModifier);
	return true;
}

bool WeaponMelee::getSkillType(const Player* player, const Item* item, skills_t& skill, uint32_t& skillpoint) const
{
	if (player->getAddAttackSkill()) {
		skillpoint = 1;
	} else {
		skillpoint = 0;
	}

	WeaponType_t weaponType = item->getWeaponType();
	switch (weaponType) {
		case WEAPON_SWORD: {
			skill = SKILL_SWORD;
			return true;
		}

		case WEAPON_CLUB: {
			skill = SKILL_CLUB;
			return true;
		}

		case WEAPON_AXE: {
			skill = SKILL_AXE;
			return true;
		}

		default:
			break;
	}
	return false;
}

int32_t WeaponMelee::getElementDamage(const Player* player, const Creature*, const Item* item) const
{
	if (elementType == COMBAT_NONE) {
		return 0;
	}

	int32_t attackSkill = player->getWeaponSkill(item);
	int32_t attackValue = elementDamage;
	float attackFactor = player->getAttackFactor();

	int32_t maxValue = Weapons::getMaxWeaponDamage(player->getLevel(), attackSkill, attackValue, attackFactor);

	if (g_config.getBoolean(ConfigManager::USE_CLASSIC_COMBAT_FORMULAS)) {
		return -(static_cast<int32_t>(maxValue * player->getVocation()->meleeDamageMultiplier));
	}

	return -uniform_random(0, static_cast<int32_t>(maxValue * player->getVocation()->meleeDamageMultiplier));
}

int32_t WeaponMelee::getWeaponDamage(const Player* player, const Creature*, const Item* item,
                                     bool maxDamage /*= false*/) const
{
	int32_t attackSkill = player->getWeaponSkill(item);
	int32_t attackValue = std::max<int32_t>(0, item->getAttack());
	float attackFactor = player->getAttackFactor();

	int32_t maxValue =
	    static_cast<int32_t>(Weapons::getMaxWeaponDamage(player->getLevel(), attackSkill, attackValue, attackFactor) *
	                         player->getVocation()->meleeDamageMultiplier);

	if (maxDamage) {
		return -maxValue;
	}

	if (g_config.getBoolean(ConfigManager::USE_CLASSIC_COMBAT_FORMULAS)) {
		return -maxValue;
	} else {
		return -uniform_random(0, maxValue);
	}
}

WeaponDistance::WeaponDistance(LuaScriptInterface* interface) : Weapon(interface)
{
	params.blockedByArmor = true;
	params.combatType = COMBAT_PHYSICALDAMAGE;
}

void WeaponDistance::configureWeapon(const ItemType& it)
{
	params.distanceEffect = it.shootType;

	if (it.abilities) {
		elementType = it.abilities->elementType;
		elementDamage = it.abilities->elementDamage;
		params.aggressive = true;
		params.useCharges = true;
	} else {
		elementType = COMBAT_NONE;
		elementDamage = 0;
	}

	if (it.conditionDamage) {
		params.conditionList.emplace_front(it.conditionDamage->clone());
	}

	Weapon::configureWeapon(it);
}

bool WeaponDistance::useWeapon(Player* player, Item* item, Creature* target) const
{
	int32_t damageModifier = 0;
	const ItemType& it = Item::items[id];
	if (it.weaponType == WEAPON_AMMO) {
		Item* mainWeaponItem = player->getWeapon(true);
		const Weapon* mainWeapon = g_weapons->getWeapon(mainWeaponItem);
		if (mainWeapon) {
			damageModifier = mainWeapon->playerWeaponCheck(player, target, mainWeaponItem->getShootRange());
		} else if (mainWeaponItem) {
			damageModifier = playerWeaponCheck(player, target, mainWeaponItem->getShootRange());
		}
	} else {
		damageModifier = playerWeaponCheck(player, target, item->getShootRange());
	}

	if (damageModifier == 0) {
		return false;
	}

	int32_t chance = 0;
	if (it.hitChance != 0) {
		chance = it.hitChance;
	}

	if (item->getWeaponType() == WEAPON_AMMO) {
		Item* bow = player->getWeapon(true);
		if (bow && bow->getHitChance() != 0) {
			chance += bow->getHitChance();
		}
	}

	if (it.ammoType != AMMO_NONE) {
		// chance on two-handed weapons is 90%
		chance += 90;
	} else {
		// chance on throwables is 75%
		chance += 75;
	}

	// maximum of 100% chance
	chance = std::min<int32_t>(100, chance);

	int32_t skill = player->getSkillLevel(SKILL_DISTANCE);
	const Position& playerPos = player->getPosition();
	const Position& targetPos = target->getPosition();
	uint32_t distance =
	    std::max<uint32_t>(Position::getDistanceX(playerPos, targetPos), Position::getDistanceY(playerPos, targetPos));

	if (distance <= 1) {
		distance = 5;
	}

	bool hit = false;
	if (chance) {
		hit = static_cast<int32_t>(rand() % (distance * 15)) <= skill && rand() % 100 <= chance;
	}

	if (hit) {
		const Item *shield, *weapon;
		player->getShieldAndWeapon(shield, weapon);

		if (shield) {
			if (target && target->blockCount > 0) {
				target->blockCount--;
			}
		}

		Weapon::internalUseWeapon(player, item, target, damageModifier);
	} else {
		player->setLastAttackBlockType(BLOCK_DEFENSE);
		player->decrementBloodHitCount(0);

		// miss target
		Tile* destTile = target->getTile();

		if (!Position::areInRange<1, 1, 0>(player->getPosition(), target->getPosition())) {
			static std::vector<std::pair<int32_t, int32_t>> destList{{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {0, 0},
			                                                         {1, 0},   {-1, 1}, {0, 1},  {1, 1}};
			std::shuffle(destList.begin(), destList.end(), getRandomGenerator());

			Position destPos = target->getPosition();

			for (const auto& dir : destList) {
				// Blocking tiles or tiles without ground ain't valid targets for spears
				Tile* tmpTile = g_game.map.getTile(destPos.x + dir.first, destPos.y + dir.second, destPos.z);
				if (tmpTile && !tmpTile->hasFlag(TILESTATE_BLOCKSOLID) && tmpTile->getGround() != nullptr) {
					destTile = tmpTile;
					break;
				}
			}
		}

		Weapon::internalUseWeapon(player, item, destTile, false);
	}
	return true;
}

int32_t WeaponDistance::getElementDamage(const Player* player, const Creature* target, const Item* item) const
{
	if (elementType == COMBAT_NONE) {
		return 0;
	}

	int32_t attackValue = elementDamage;
	if (item->getWeaponType() == WEAPON_AMMO) {
		Item* weapon = player->getWeapon(true);
		if (weapon) {
			attackValue += weapon->getAttack();
		}
	}

	int32_t attackSkill = player->getSkillLevel(SKILL_DISTANCE);
	float attackFactor = player->getAttackFactor();

	int32_t minValue = 0;
	int32_t maxValue = Weapons::getMaxWeaponDamage(player->getLevel(), attackSkill, attackValue, attackFactor);

	if (target) {
		if (target->getPlayer()) {
			minValue = static_cast<int32_t>(std::ceil(player->getLevel() * 0.1));
		} else {
			minValue = static_cast<int32_t>(std::ceil(player->getLevel() * 0.2));
		}
	}

	if (g_config.getBoolean(ConfigManager::USE_CLASSIC_COMBAT_FORMULAS)) {
		return -(static_cast<int32_t>(maxValue * player->getVocation()->distDamageMultiplier));
	} else {
		return -uniform_random(minValue, static_cast<int32_t>(maxValue * player->getVocation()->distDamageMultiplier));
	}
}

int32_t WeaponDistance::getWeaponDamage(const Player* player, const Creature* target, const Item* item,
                                        bool maxDamage /*= false*/) const
{
	int32_t attackValue = item->getAttack();

	if (item->getWeaponType() == WEAPON_AMMO) {
		Item* weapon = player->getWeapon(true);
		if (weapon) {
			attackValue += weapon->getAttack();
		}
	}

	int32_t attackSkill = player->getSkillLevel(SKILL_DISTANCE);
	float attackFactor = player->getAttackFactor();

	int32_t maxValue =
	    static_cast<int32_t>(Weapons::getMaxWeaponDamage(player->getLevel(), attackSkill, attackValue, attackFactor) *
	                         player->getVocation()->distDamageMultiplier);

	if (maxDamage) {
		return -maxValue;
	}

	int32_t minValue;
	if (target) {
		if (target->getPlayer()) {
			minValue = static_cast<int32_t>(std::ceil(player->getLevel() * 0.1));
		} else {
			minValue = static_cast<int32_t>(std::ceil(player->getLevel() * 0.2));
		}
	} else {
		minValue = 0;
	}

	if (g_config.getBoolean(ConfigManager::USE_CLASSIC_COMBAT_FORMULAS)) {
		return -maxValue;
	} else {
		return -uniform_random(minValue, maxValue);
	}
}

bool WeaponDistance::getSkillType(const Player* player, const Item*, skills_t& skill, uint32_t& skillpoint) const
{
	skill = SKILL_DISTANCE;

	if (player->getAddAttackSkill()) {
		switch (player->getLastAttackBlockType()) {
			case BLOCK_IMMUNITY:
			case BLOCK_ARMOR:
			case BLOCK_NONE: {
				skillpoint = 2;
				break;
			}

			case BLOCK_DEFENSE: {
				skillpoint = 1;
				break;
			}

			default:
				skillpoint = 0;
				break;
		}
	} else {
		skillpoint = 0;
	}
	return true;
}

void WeaponWand::configureWeapon(const ItemType& it)
{
	params.distanceEffect = it.shootType;

	Weapon::configureWeapon(it);
}

int32_t WeaponWand::getWeaponDamage(const Player*, const Creature*, const Item*, bool maxDamage /*= false*/) const
{
	if (maxDamage) {
		return -maxChange;
	}
	return -uniform_random(minChange, maxChange);
}
