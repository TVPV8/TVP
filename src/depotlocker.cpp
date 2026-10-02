// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "depotlocker.h"

#include <fmt/format.h>

DepotLocker::DepotLocker(uint16_t type) : Container(type, 30), depotId(0), maxDepotItems(2000) {}

Item* DepotLocker::clone() const
{
	DepotLocker* clone = static_cast<DepotLocker*>(Item::clone());
	clone->setDepotId(depotId);
	clone->setMaxDepotItems(maxDepotItems);
	return clone;
}

Attr_ReadValue DepotLocker::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	if (attr == ATTR_DEPOT_ID) {
		if (!propStream.read<uint16_t>(depotId)) {
			return ATTR_READ_ERROR;
		}
		return ATTR_READ_CONTINUE;
	}
	return Item::readAttr(attr, propStream);
}

void DepotLocker::serializeAttr(PropWriteStream& propWriteStream) const
{
	Item::serializeAttr(propWriteStream);

	propWriteStream.write<uint8_t>(ATTR_DEPOT_ID);
	propWriteStream.write<uint16_t>(depotId);
}

ReturnValue DepotLocker::queryAdd(int32_t index, const Thing& thing, uint32_t count, uint32_t flags,
                                  Creature* actor /* = nullptr*/) const
{
	const Item* item = thing.getItem();
	if (item == nullptr) {
		return RETURNVALUE_NOTPOSSIBLE;
	}

	bool skipLimit = hasBitSet(FLAG_NOLIMIT, flags);
	if (!skipLimit) {
		const bool depotBeyondFull = getItemHoldingCount() > maxDepotItems;
		const bool isInsideDepotLocker = item->getHoldingDepot() != nullptr;

		int32_t addCount = 1;

		if (const Container* container = item->getContainer()) {
			addCount = container->getItemHoldingCount() + 1;
		}

		bool movingToStack = false;
		if (item->isStackable()) {
			const Item* itemAtIndex = getItemByIndex(index);
			if (itemAtIndex && itemAtIndex->equals(item) && itemAtIndex->getItemCount() < 100) {
				if (depotBeyondFull && isInsideDepotLocker || !depotBeyondFull) {
					addCount = 0;
				}
			}
		}

		if (isInsideDepotLocker) {
			if (!item->isStackable() || item->getItemCount() == count) {
				addCount = 0;
			}
		}

		if (addCount != 0 && getItemHoldingCount() + addCount > maxDepotItems) {
			return RETURNVALUE_DEPOTISFULL;
		}
	}

	return Container::queryAdd(index, thing, count, flags, actor);
}

void DepotLocker::postAddNotification(Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t)
{
	if (parent != nullptr) {
		parent->postAddNotification(thing, oldParent, index, LINK_PARENT);
	}
}

void DepotLocker::postRemoveNotification(Thing* thing, const Cylinder* newParent, int32_t index, cylinderlink_t)
{
	if (parent != nullptr) {
		parent->postRemoveNotification(thing, newParent, index, LINK_PARENT);
	}
}
