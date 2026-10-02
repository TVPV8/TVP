// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "actions.h"
#include "bed.h"
#include "configmanager.h"
#include "container.h"
#include "game.h"
#include "pugicast.h"
#include "spells.h"
#include "events.h"
#include <fmt/format.h>

extern Game g_game;
extern Spells* g_spells;
extern Actions* g_actions;
extern ConfigManager g_config;
extern Events* g_events;

Actions::Actions() : scriptInterface("Action Interface") { scriptInterface.initState(); }

Actions::~Actions() { clear(); }

void Actions::clearMap(ActionUseMap& map) { map.clear(); }

void Actions::clear()
{
	clearMap(useItemMap);
	clearMap(uniqueItemMap);
	clearMap(actionItemMap);

	getScriptInterface().reInitState();
}

LuaScriptInterface& Actions::getScriptInterface() { return scriptInterface; }

std::string Actions::getScriptBaseName() const { return "actions"; }

bool Actions::registerLuaEvent(Action* event)
{
	Action_ptr action{event};
	if (!action->getItemIdRange().empty()) {
		const auto& range = action->getItemIdRange();
		for (auto id : range) {
			auto result = useItemMap.emplace(id, *action);
			if (!result.second) {
				std::cout << "[Warning - Actions::registerLuaEvent] Duplicate registered item with id: " << id
				          << " in range from id: " << range.front() << ", to id: " << range.back() << std::endl;
			}
		}
		return true;
	}

	if (!action->getUniqueIdRange().empty()) {
		const auto& range = action->getUniqueIdRange();
		for (auto id : range) {
			auto result = uniqueItemMap.emplace(id, *action);
			if (!result.second) {
				std::cout << "[Warning - Actions::registerLuaEvent] Duplicate registered item with uid: " << id
				          << " in range from uid: " << range.front() << ", to uid: " << range.back() << std::endl;
			}
		}
		return true;
	}

	if (!action->getActionIdRange().empty()) {
		const auto& range = action->getActionIdRange();
		for (auto id : range) {
			auto result = actionItemMap.emplace(id, *action);
			if (!result.second) {
				std::cout << "[Warning - Actions::registerLuaEvent] Duplicate registered item with aid: " << id
				          << " in range from aid: " << range.front() << ", to aid: " << range.back() << std::endl;
			}
		}
		return true;
	}

	std::cout << "[Warning - Actions::registerLuaEvent] There is no id / aid / uid set for this event" << std::endl;
	return false;
}

ReturnValue Actions::canUse(const Player* player, const Position& pos)
{
	if (pos.x != 0xFFFF) {
		const Position& playerPos = player->getPosition();
		if (playerPos.z != pos.z) {
			return playerPos.z > pos.z ? RETURNVALUE_FIRSTGOUPSTAIRS : RETURNVALUE_FIRSTGODOWNSTAIRS;
		}

		if (!Position::areInRange<1, 1>(playerPos, pos)) {
			return RETURNVALUE_TOOFARAWAY;
		}
	}
	return RETURNVALUE_NOERROR;
}

ReturnValue Actions::canUse(const Player* player, const Position& pos, const Item* item)
{
	Action* action = getAction(item);
	if (action) {
		return action->canExecuteAction(player, pos);
	}
	return RETURNVALUE_NOERROR;
}

ReturnValue Actions::canUseFar(const Creature* creature, const Position& toPos, bool checkLineOfSight, bool checkFloor,
                               bool isRune)
{
	if (toPos.x == 0xFFFF) {
		return RETURNVALUE_NOERROR;
	}

	const Position& creaturePos = creature->getPosition();
	if (checkFloor && creaturePos.z != toPos.z) {
		return creaturePos.z > toPos.z ? RETURNVALUE_FIRSTGOUPSTAIRS : RETURNVALUE_FIRSTGODOWNSTAIRS;
	}

	if (!isRune) {
		if (!Position::areInRange<7, 5>(toPos, creaturePos)) {
			return RETURNVALUE_TOOFARAWAY;
		}
	} else {
		if (!Position::areInRange<7, 7>(toPos, creaturePos)) {
			return RETURNVALUE_TOOFARAWAY;
		}
	}

	if (checkLineOfSight && !g_game.canThrowObjectTo(creaturePos, toPos, false)) {
		return RETURNVALUE_CANNOTTHROW;
	}

	return RETURNVALUE_NOERROR;
}

Action* Actions::getAction(const Item* item)
{
	if (item->hasAttribute(ITEM_ATTRIBUTE_UNIQUEID)) {
		auto it = uniqueItemMap.find(item->getUniqueId());
		if (it != uniqueItemMap.end()) {
			return &it->second;
		}
	}

	if (item->hasAttribute(ITEM_ATTRIBUTE_ACTIONID)) {
		auto it = actionItemMap.find(item->getActionId());
		if (it != actionItemMap.end()) {
			return &it->second;
		}
	}

	auto it = useItemMap.find(item->getID());
	if (it != useItemMap.end()) {
		return &it->second;
	}

	// rune items
	return g_spells->getRuneSpell(item->getID());
}

ReturnValue Actions::internalUseItem(Player* player, const Position& pos, uint8_t index, Item* item)
{
	if (Door* door = item->getDoor()) {
		if (!door->canUse(player)) {
			return RETURNVALUE_NOTPOSSIBLE;
		}
	}

	Action* action = getAction(item);
	if (action) {
		if (action->executeUse(player, item, pos, nullptr, pos)) {
			return RETURNVALUE_NOERROR;
		}

		if (item->isRemoved()) {
			return RETURNVALUE_CANNOTUSETHISOBJECT;
		}
	}

	if (BedItem* bed = item->getBed()) {
		if (!bed->canUse(player)) {
			if (!bed->getHouse()) {
				return RETURNVALUE_CANNOTUSETHISOBJECT;
			}

			if (!player->isPremium()) {
				return RETURNVALUE_YOUNEEDPREMIUMACCOUNT;
			}

			return RETURNVALUE_CANNOTUSETHISOBJECT;
		}

		if (bed->trySleep(player)) {
			bed->sleep(player);
		}

		return RETURNVALUE_NOERROR;
	}

	if (Container* container = item->getContainer()) {
		Container* openContainer;

		// Depot container
		if (container->getDepotLocker()) {
			if (DepotLocker* myDepotLocker = player->currentDepotItem) {
				openContainer = myDepotLocker;

				if (myDepotLocker->getItemTypeCount(ITEM_DEPOT) == 0) {
					myDepotLocker->addItem(Item::CreateItem(ITEM_DEPOT, 1));
				}
			} else {
				// Open depot as normal container
				openContainer = container;
			}
		} else {
			openContainer = container;
		}

		/*uint32_t corpseOwner = container->getCorpseOwner();
		if (corpseOwner != 0 && !player->canOpenCorpse(corpseOwner)) {
		    return RETURNVALUE_YOUARENOTTHEOWNER;
		}*/

		// open/close container
		int32_t oldContainerId = player->getContainerID(openContainer);
		if (oldContainerId == -1) {
			player->addContainer(index, openContainer);
			player->onSendContainer(openContainer);
		} else {
			player->onCloseContainer(openContainer);
			player->closeContainer(oldContainerId);
		}

		return RETURNVALUE_NOERROR;
	}

	const ItemType& it = Item::items[item->getID()];
	if (it.canReadText) {
		if (it.canWriteText) {
			player->setWriteItem(item, it.maxTextLen);
			player->sendTextWindow(item, it.maxTextLen, true);
		} else {
			player->setWriteItem(nullptr);
			player->sendTextWindow(item, 0, false);
		}

		return RETURNVALUE_NOERROR;
	}

	if (g_events->eventPlayerOnUseItem(player, item)) {
		return RETURNVALUE_NOTPOSSIBLE;
	}

	return RETURNVALUE_CANNOTUSETHISOBJECT;
}

bool Actions::useItem(Player* player, const Position& pos, uint8_t index, Item* item)
{
	if (g_config.getBoolean(ConfigManager::ONLY_INVITED_CAN_MOVE_HOUSE_ITEMS)) {
		if (const House* house = item->getTile()->getHouse()) {
			if (!item->getTopParent()->getCreature() && !house->isInvited(player)) {
				player->sendCancelMessage(RETURNVALUE_PLAYERISNOTINVITED);
				return false;
			}
		}
	}

	ReturnValue ret = internalUseItem(player, pos, index, item);
	if (ret != RETURNVALUE_NOERROR) {
		player->sendCancelMessage(ret);
		return false;
	}

	return true;
}

bool Actions::useItemEx(Player* player, const Position& fromPos, const Position& toPos, uint8_t toStackPos,
                        uint16_t toSpriteId, Item* item, Creature* creature /* = nullptr*/)
{
	Action* action = getAction(item);
	if (!action) {
		player->sendCancelMessage(RETURNVALUE_CANNOTUSETHISOBJECT);
		return false;
	}

	ReturnValue ret = action->canExecuteAction(player, toPos);
	if (ret != RETURNVALUE_NOERROR) {
		player->sendCancelMessage(ret);
		return false;
	}

	Thing* target = action->getTarget(player, creature, toPos, toStackPos, toSpriteId);
	if (target) {
		// ** OTC related fix only ** //
		// OTC should not allow using items on splashes like classic Tibia client
		if (Item::items.getItemIdByClientId(toSpriteId).isSplash()) {
			if (Item* item = target->getItem()) {
				toSpriteId = Item::items.getItemType(item->getID()).clientId;
			}
		}

		// ** Tibia Client related fix only ** //
		if (toSpriteId > 99 && toPos == player->getPosition() || target->getCreature()) {
			toSpriteId = 99; // Override use in creature instead
		}

		if (target->getCreature() && toSpriteId > 99) {
			player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
			return false;
		}

		if (const Item* targetItem = target->getItem()) {
			if (Item::items[targetItem->getID()].clientId != toSpriteId) {
				player->sendCancelMessage(RETURNVALUE_NOTPOSSIBLE);
				return false;
			}
		}
	}

	if (g_config.getBoolean(ConfigManager::ONLY_INVITED_CAN_MOVE_HOUSE_ITEMS)) {
		if (const House* house = item->getTile()->getHouse()) {
			if (!item->getTopParent()->getCreature() && !house->isInvited(player)) {
				player->sendCancelMessage(RETURNVALUE_PLAYERISNOTINVITED);
				return false;
			}
		}
	}

	if (action->executeUse(player, item, fromPos, target, toPos)) {
		return true;
	}

	if (!action->hasOwnErrorHandler()) {
		player->sendCancelMessage(RETURNVALUE_CANNOTUSETHISOBJECT);
	}

	return false;
}

Action::Action(LuaScriptInterface* interface) :
    ScriptEvent(interface), allowFarUse(false), checkFloor(true), checkLineOfSight(true)
{}

std::string Action::getScriptEventName() const { return "onUse"; }

ReturnValue Action::canExecuteAction(const Player* player, const Position& toPos)
{
	if (allowFarUse) {
		return g_actions->canUseFar(player, toPos, checkLineOfSight, checkFloor,
		                            dynamic_cast<RuneSpell*>(this) != nullptr);
	}
	return g_actions->canUse(player, toPos);
}

Thing* Action::getTarget(Player* player, Creature* targetCreature, const Position& toPosition, uint8_t toStackPos,
                         uint16_t spriteId) const
{
	if (targetCreature) {
		return targetCreature;
	}

	Thing* thing = g_game.internalGetThing(player, toPosition, toStackPos, spriteId, STACKPOS_USETARGET);
	if (thing) {
		if (Item* item = thing->getItem()) {
			if (Item::items[item->getID()].clientId != spriteId) {
				const ItemType& iType = Item::items.getItemIdByClientId(spriteId);
				item = g_game.findItemOfType(g_game.internalGetCylinder(player, toPosition), iType.id);
				if (item) {
					return item;
				}
			}
		}
	}
	return thing;
}

bool Action::executeUse(Player* player, Item* item, const Position& fromPosition, Thing* target,
                        const Position& toPosition)
{
	// onUse(player, item, fromPosition, target, toPosition)
	if (!scriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Action::executeUse] Call stack overflow" << std::endl;
		return false;
	}

	ScriptEnvironment* env = scriptInterface->getScriptEnv();
	env->setScriptId(scriptId, scriptInterface);

	lua_State* L = scriptInterface->getLuaState();

	scriptInterface->pushFunction(scriptId);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushThing(L, item);
	LuaScriptInterface::pushPosition(L, fromPosition);

	LuaScriptInterface::pushThing(L, target);
	LuaScriptInterface::pushPosition(L, toPosition);

	return scriptInterface->callFunction(5);
}
