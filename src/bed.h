// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include "item.h"

class House;
class Player;

class BedItem final : public Item
{
public:
	explicit BedItem(uint16_t id) : Item(id) {}

	BedItem* getBed() override { return this; }
	const BedItem* getBed() const override { return this; }

	Attr_ReadValue readAttr(AttrTypes_t attr, PropStream& propStream) override;
	void serializeAttr(PropWriteStream& propWriteStream) const override;

	void setSleeper(uint32_t guid) { sleeperGUID = guid; }
	uint32_t getSleeper() const { return sleeperGUID; }

	House* getHouse() const { return house; }
	void setHouse(House* h) { house = h; }

	bool canUse(Player* player);

	bool trySleep(Player* player);
	bool sleep(Player* player);
	void wakeUp(Player* player);

	BedItem* getNextBedItem() const;

protected:
	void updateAppearance(const Player* player);
	void regeneratePlayer(Player* player) const;
	void internalSetSleeper(const Player* player);
	void internalRemoveSleeper();

	House* house = nullptr;
	uint64_t sleepStart = 0;
	uint32_t sleeperGUID = 0;

	friend class Item;
};
