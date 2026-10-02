// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include "container.h"

using DepotLocker_ptr = std::shared_ptr<DepotLocker>;

class DepotLocker final : public Container
{
public:
	explicit DepotLocker(uint16_t type);

	// serialization
	uint32_t getMaxDepotItems() const { return maxDepotItems; }
	void setMaxDepotItems(uint32_t maxitems) { maxDepotItems = maxitems; }

	DepotLocker* getDepotLocker() override { return this; }
	const DepotLocker* getDepotLocker() const override { return this; }

	Item* clone() const override final;

	// serialization
	Attr_ReadValue readAttr(AttrTypes_t attr, PropStream& propStream) override;
	void serializeAttr(PropWriteStream& propWriteStream) const override;

	uint16_t getDepotId() const { return depotId; }
	void setDepotId(uint16_t depotId) { this->depotId = depotId; }

	// cylinder implementations
	ReturnValue queryAdd(int32_t index, const Thing& thing, uint32_t count, uint32_t flags,
	                     Creature* actor = nullptr) const override;

	void postAddNotification(Thing* thing, const Cylinder* oldParent, int32_t index,
	                         cylinderlink_t link = LINK_OWNER) override;
	void postRemoveNotification(Thing* thing, const Cylinder* newParent, int32_t index,
	                            cylinderlink_t link = LINK_OWNER) override;

	bool hasLoadedContent() const { return isLoaded; }
	void toggleIsLoaded(bool value) { isLoaded = value; }

private:
	uint16_t depotId;
	uint32_t maxDepotItems;

	bool isLoaded = false;
};
