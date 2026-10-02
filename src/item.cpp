// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "item.h"
#include "container.h"
#include "teleport.h"
#include "trashholder.h"
#include "mailbox.h"
#include "house.h"
#include "game.h"
#include "bed.h"
#include "actions.h"
#include "spells.h"
#include "scriptwriter.h"
#include "iomap.h"

#include <fmt/format.h>

extern Game g_game;
extern Spells* g_spells;
extern Vocations g_vocations;

Items Item::items;

Item* Item::CreateItem(const uint16_t type, uint16_t count /*= 0*/)
{
	Item* newItem = nullptr;

	const ItemType& it = Item::items[type];
	if (it.group == ITEM_GROUP_DEPRECATED) {
		return nullptr;
	}

	if (it.stackable && count == 0) {
		count = 1;
	}

	if (it.id != 0) {
		if (it.isDepot()) {
			newItem = new DepotLocker(type);
		} else if (it.isContainer()) {
			newItem = new Container(type);
		} else if (it.isTeleport()) {
			newItem = new Teleport(type);
		} else if (it.isMagicField()) {
			newItem = new MagicField(type);
		} else if (it.isDoor()) {
			newItem = new Door(type);
		} else if (it.isTrashHolder()) {
			newItem = new TrashHolder(type);
		} else if (it.isMailbox()) {
			newItem = new Mailbox(type);
		} else if (it.isBed()) {
			newItem = new BedItem(type);
		} else {
			newItem = new Item(type, count);
		}

		newItem->incrementReferenceCounter();
	}

	return newItem;
}

Container* Item::CreateItemAsContainer(const uint16_t type, uint16_t size)
{
	const ItemType& it = Item::items[type];
	if (it.id == 0 || it.group == ITEM_GROUP_DEPRECATED || it.stackable || it.useable || it.moveable || it.pickupable ||
	    it.isDepot() || it.isSplash() || it.isDoor()) {
		return nullptr;
	}

	Container* newItem = new Container(type, size);
	newItem->incrementReferenceCounter();
	return newItem;
}

Item* Item::CreateItem(PropStream& propStream)
{
	uint16_t id;
	if (!propStream.read<uint16_t>(id)) {
		return nullptr;
	}

	return Item::CreateItem(id, 0);
}

Item* Item::CreateItem(ScriptReader& scriptReader)
{
	uint16_t id = scriptReader.getNumber();
	return Item::CreateItem(id, 0);
}

Item::Item(const uint16_t type, uint16_t count /*= 0*/) : id(type)
{
	const ItemType& it = items[id];

	if (it.isFluidContainer() || it.isSplash()) {
		setFluidType(count);
	} else if (it.stackable) {
		if (count != 0) {
			setItemCount(count);
		} else if (it.charges != 0) {
			setItemCount(it.charges);
		}
	} else if (it.charges != 0 || it.isRune()) {
		if (count != 0) {
			setCharges(count);
		} else {
			setCharges(it.charges);
		}
	} else if (it.isKey()) {
		setKeyNumber(count);
	}

	setDefaultDuration();
}

Item::Item(const Item& i) : Thing(), id(i.id), count(i.count)
{
	if (i.attributes) {
		attributes.reset(new ItemAttributes(*i.attributes));
	}
}

Item* Item::clone() const
{
	Item* item = Item::CreateItem(id, count);
	if (attributes) {
		item->attributes.reset(new ItemAttributes(*attributes));
		if (item->getDuration() > 0) {
			item->incrementReferenceCounter();
			item->setDecaying(DECAYING_TRUE);
			g_game.toDecayItems.push_front(item);
		}
	}

	return item;
}

bool Item::equals(const Item* otherItem) const
{
	if (!otherItem || id != otherItem->id) {
		return false;
	}

	const auto& otherAttributes = otherItem->attributes;
	if (!attributes) {
		return !otherAttributes || (otherAttributes->attributeBits == 0);
	} else if (!otherAttributes) {
		return (attributes->attributeBits == 0);
	}

	if (attributes->attributeBits != otherAttributes->attributeBits) {
		return false;
	}

	const auto& attributeList = attributes->attributes;
	const auto& otherAttributeList = otherAttributes->attributes;
	for (const auto& attribute : attributeList) {
		if (ItemAttributes::isStrAttrType(attribute.type)) {
			for (const auto& otherAttribute : otherAttributeList) {
				if (attribute.type == otherAttribute.type && *attribute.value.string != *otherAttribute.value.string) {
					return false;
				}
			}
		} else {
			for (const auto& otherAttribute : otherAttributeList) {
				if (attribute.type == otherAttribute.type && attribute.value.integer != otherAttribute.value.integer) {
					return false;
				}
			}
		}
	}
	return true;
}

void Item::setDefaultSubtype()
{
	const ItemType& it = items[id];

	setItemCount(1);

	if (it.charges != 0) {
		if (it.stackable) {
			setItemCount(it.charges);
		} else {
			setCharges(it.charges);
		}
	}
}

void Item::onRemoved()
{
	ScriptEnvironment::removeTempItem(this);

	if (hasAttribute(ITEM_ATTRIBUTE_UNIQUEID)) {
		g_game.removeUniqueItem(getUniqueId());
	}
}

void Item::setID(uint16_t newid)
{
	const ItemType& prevIt = Item::items[id];
	id = newid;

	const ItemType& it = Item::items[newid];
	uint32_t newDuration = it.decayTime * 1000;

	if (newDuration == 0 && !it.stopTime && it.decayTo < 0) {
		removeAttribute(ITEM_ATTRIBUTE_DECAYSTATE);
		removeAttribute(ITEM_ATTRIBUTE_DURATION);
	}

	removeAttribute(ITEM_ATTRIBUTE_CORPSEOWNER);

	if (newDuration > 0 && (!prevIt.stopTime || !hasAttribute(ITEM_ATTRIBUTE_DURATION))) {
		setDecaying(DECAYING_FALSE);
		setDuration(newDuration);
	}
}

void Item::decrementReferenceCounter()
{
	if (--referenceCounter == 0) {
		// In the given situation that item is to be deleted but still has a parent, skip deletion
		if (Cylinder* parentCylinder = getParent()) {
			if (parentCylinder != VirtualCylinder::virtualCylinder) {
				bool parentTile = parentCylinder->getTile() != nullptr;
				bool parentContainer = parentCylinder->getContainer() != nullptr;
				bool parentPlayer = !parentTile && !parentContainer;

				const Position& pos = getPosition();
				std::cout
				    << fmt::format(
				           "ERROR - Item::decrementReferenceCounter: Item was to be deleted {:s}:{:d} ~ ({:d},{:d},{:d}) ~ Pos ({:d},{:d},{:d}) but is still present in a parent cylinder.",
				           getName(), getID(), parentTile, parentContainer, parentPlayer, pos.x, pos.y,
				           static_cast<int>(pos.z))
				    << std::endl;
				// Since reference counter will be 1
				// It will be deleted during cylinder destructors anyway and proceed without warning
				// And if it was not to be deleted, it will be less than 10 bytes kept in memory.
				referenceCounter++;
				return;
			}
		}
		delete this;
	}
}

Cylinder* Item::getTopParent()
{
	Cylinder* aux = getParent();
	Cylinder* prevaux = dynamic_cast<Cylinder*>(this);
	if (!aux) {
		return prevaux;
	}

	while (aux->getParent() != nullptr) {
		prevaux = aux;
		aux = aux->getParent();
	}

	if (prevaux) {
		return prevaux;
	}
	return aux;
}

const Cylinder* Item::getTopParent() const
{
	const Cylinder* aux = getParent();
	const Cylinder* prevaux = dynamic_cast<const Cylinder*>(this);
	if (!aux) {
		return prevaux;
	}

	while (aux->getParent() != nullptr) {
		prevaux = aux;
		aux = aux->getParent();
	}

	if (prevaux) {
		return prevaux;
	}
	return aux;
}

Tile* Item::getTile()
{
	Cylinder* cylinder = getTopParent();
	// get root cylinder
	if (cylinder && cylinder->getParent()) {
		cylinder = cylinder->getParent();
	}
	return dynamic_cast<Tile*>(cylinder);
}

const Tile* Item::getTile() const
{
	const Cylinder* cylinder = getTopParent();
	// get root cylinder
	if (cylinder && cylinder->getParent()) {
		cylinder = cylinder->getParent();
	}
	return dynamic_cast<const Tile*>(cylinder);
}

bool Item::isRemoved() const
{
	// A child item is querying this call
	if (const Container* container = getContainer()) {
		if (const DepotLocker* depotLocker = container->getDepotLocker()) {
			if (g_config.getBoolean(ConfigManager::ITEMS_DECAY_INSIDE_DEPOTS)) {
				return false;
			}
		}
	}

	if (!parent) {
		return true;
	}

	if (Container* container = parent->getContainer()) {
		// If parent is a depot locker, the parent item is never removed
		if (DepotLocker* depotLocker = container->getDepotLocker()) {
			return !depotLocker->hasLoadedContent();
		}
	}

	return parent->isRemoved();
}

uint16_t Item::getSubType() const
{
	const ItemType& it = items[id];
	if (it.isFluidContainer() || it.isSplash()) {
		return getFluidType();
	} else if (it.stackable) {
		return count;
	} else if (it.charges != 0 || it.isRune()) {
		return getCharges();
	}
	return count;
}

Player* Item::getHoldingPlayer() const
{
	Cylinder* p = getParent();
	while (p) {
		if (p->getCreature()) {
			return p->getCreature()->getPlayer();
		}

		p = p->getParent();
	}
	return nullptr;
}

DepotLocker* Item::getHoldingDepot()
{
	Cylinder* p = getParent();
	while (p) {
		if (Item* item = p->getItem()) {
			if (Container* container = item->getContainer()) {
				if (DepotLocker* depot = container->getDepotLocker()) {
					return depot;
				}
			}
		}

		p = p->getParent();
	}
	return nullptr;
}

const DepotLocker* Item::getHoldingDepot() const
{
	const Cylinder* p = getParent();
	while (p) {
		if (const Item* item = p->getItem()) {
			if (const Container* container = item->getContainer()) {
				if (const DepotLocker* depot = container->getDepotLocker()) {
					return depot;
				}
			}
		}

		p = p->getParent();
	}
	return nullptr;
}

void Item::setSubType(uint16_t n)
{
	const ItemType& it = items[id];
	if (it.isFluidContainer() || it.isSplash()) {
		setFluidType(n);
	} else if (it.stackable) {
		setItemCount(n);
	} else if (it.charges != 0 || it.isRune()) {
		setCharges(n);
	} else {
		setItemCount(n);
	}
}

bool Item::unserializeTVPFormat(ScriptReader& script)
{
	while (script.canRead()) {
		script.nextToken();
		if (script.getToken() != TOKEN_IDENTIFIER) {
			break;
		}

		std::string identifier = script.getIdentifier();
		script.readSymbol('=');
		if (identifier == "amount") {
			setItemCount(script.readNumber());
		} else if (identifier == "fluidtype") {
			setSubType(script.readNumber());
		} else if (identifier == "charges") {
			setCharges(script.readNumber());
		} else if (identifier == "actionid") {
			setActionId(script.readNumber());
		} else if (identifier == "text") {
			setText(script.prepString(script.readString()));
		} else if (identifier == "writtendate") {
			setDate(script.readNumber());
		} else if (identifier == "writtenby") {
			setWriter(script.prepString(script.readString()));
		} else if (identifier == "description") {
			setSpecialDescription(script.prepString(script.readString()));
		} else if (identifier == "duration") {
			setDuration(script.readNumber());
		} else if (identifier == "decaystate") {
			setDecaying(static_cast<ItemDecayState_t>(script.readNumber()));
			if (getDecaying() == DECAYING_TRUE) {
				setDecaying(DECAYING_PENDING);
			}
			startDecaying();
		} else if (identifier == "name") {
			setStrAttr(ITEM_ATTRIBUTE_NAME, script.prepString(script.readString()));
		} else if (identifier == "pluralname") {
			setStrAttr(ITEM_ATTRIBUTE_PLURALNAME, script.prepString(script.readString()));
		} else if (identifier == "article") {
			setStrAttr(ITEM_ATTRIBUTE_ARTICLE, script.prepString(script.readString()));
		} else if (identifier == "weight") {
			setIntAttr(ITEM_ATTRIBUTE_WEIGHT, script.readNumber());
		} else if (identifier == "attack") {
			setIntAttr(ITEM_ATTRIBUTE_ATTACK, script.readNumber());
		} else if (identifier == "defense") {
			setIntAttr(ITEM_ATTRIBUTE_DEFENSE, script.readNumber());
		} else if (identifier == "attackspeed") {
			setIntAttr(ITEM_ATTRIBUTE_ATTACK_SPEED, script.readNumber());
		} else if (identifier == "extradefense") {
			setIntAttr(ITEM_ATTRIBUTE_EXTRADEFENSE, script.readNumber());
		} else if (identifier == "armor") {
			setIntAttr(ITEM_ATTRIBUTE_ARMOR, script.readNumber());
		} else if (identifier == "hitchance") {
			setIntAttr(ITEM_ATTRIBUTE_HITCHANCE, script.readNumber());
		} else if (identifier == "shootrange") {
			setIntAttr(ITEM_ATTRIBUTE_SHOOTRANGE, script.readNumber());
		} else if (identifier == "decayto") {
			setIntAttr(ITEM_ATTRIBUTE_DECAYTO, script.readNumber());
		} else if (identifier == "keynumber") {
			setIntAttr(ITEM_ATTRIBUTE_KEYNUMBER, script.readNumber());
		} else if (identifier == "keyholenumber") {
			setIntAttr(ITEM_ATTRIBUTE_KEYHOLENUMBER, script.readNumber());
		} else if (identifier == "doorlevel") {
			setIntAttr(ITEM_ATTRIBUTE_DOORLEVEL, script.readNumber());
		} else if (identifier == "doorquestnumber") {
			setIntAttr(ITEM_ATTRIBUTE_DOORQUESTNUMBER, script.readNumber());
		} else if (identifier == "doorquestvalue") {
			setIntAttr(ITEM_ATTRIBUTE_DOORQUESTVALUE, script.readNumber());
		} else if (identifier == "doorid") {
			int32_t doorId = script.readNumber();
			if (Door* door = getDoor()) {
				door->setDoorId(doorId);
			}
		} else if (identifier == "destination") {
			Position destPos = script.readPosition();
			if (Teleport* teleport = getTeleport()) {
				teleport->setDestPos(destPos);
			}
		} else if (identifier == "depotid") {
			int32_t depotId = script.readNumber();
			if (Container* container = getContainer()) {
				if (DepotLocker* depotLocker = container->getDepotLocker()) {
					depotLocker->setDepotId(depotId);
				}
			}
		} else if (identifier == "sleeper") {
			int32_t sleeper = script.readNumber();
			if (BedItem* bed = getBed()) {
				bed->setSleeper(sleeper);
			}
		} else if (identifier == "customattr") {
			script.readSymbol('(');
			std::string attr;
			script.nextToken();
			if (script.getToken() == TOKEN_IDENTIFIER) {
				attr = script.getIdentifier();
			} else {
				attr = std::to_string(script.getNumber());
			}
			script.readSymbol(',');
			script.nextToken();
			boost::variant<std::string, int64_t, bool, double> value;
			switch (script.getToken()) {
				case TOKEN_IDENTIFIER:
					value = script.getIdentifier();
					break;
				case TOKEN_NUMBER:
					value = script.getNumber();
					break;
				case TOKEN_STRING:
					value = script.getString();
					break;
				default:
					script.error("expected identifier, boolean, number or string attribute value");
					break;
			}

			script.readSymbol(')');
			setCustomAttribute(attr, value);
		} else if (identifier == "content") {
			script.readSymbol('{');
			script.nextToken();
			while (script.canRead()) {
				if (script.getToken() == TOKEN_NUMBER) { // item
					Item* item = Item::CreateItem(script);
					if (!item) {
						script.error("could not create item");
						return false;
					}

					if (!item->unserializeTVPFormat(script)) {
						script.error("could not unserialize item data");
						return false;
					}

					Container* container = getContainer();
					if (!container) {
						delete item;
					} else {
						container->internalAddThing(item);
					}
				} else if (script.getSpecial() == ',') {
					script.nextToken();
					continue;
				} else {
					break;
				}
			}
		} else {
			script.error(fmt::format("unknown attribute '{:s}'", identifier));
			return false;
		}
	}

	return true;
}

void Item::serializeTVPFormat(ScriptWriter& script) const
{
	script.writeNumber(getID());

	const ItemType& it = items[id];
	if (it.stackable) {
		script.writeText(fmt::format(" Amount={:d}", getItemCount()));
	}

	if (it.isFluidContainer() || it.isSplash()) {
		script.writeText(fmt::format(" FluidType={:d}", getSubType()));
	}

	if (getCharges() != 0) {
		script.writeText(fmt::format(" Charges={:d}", getCharges()));
	}

	if (getActionId() != 0) {
		script.writeText(fmt::format(" ActionID={:d}", getActionId()));
	}

	if (!getText().empty()) {
		script.writeText(fmt::format(" Text=\"{:s}\"", ScriptWriter::prepString(getText())));
	}

	if (getDate() != 0) {
		script.writeText(fmt::format(" WrittenDate={:d}", getDate()));
	}

	if (!getWriter().empty()) {
		script.writeText(fmt::format(" WrittenBy=\"{:s}\"", ScriptWriter::prepString(getWriter())));
	}

	if (!getSpecialDescription().empty()) {
		script.writeText(fmt::format(" Description=\"{:s}\"", ScriptWriter::prepString(getSpecialDescription())));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_DURATION)) {
		script.writeText(fmt::format(" Duration={:d}", getIntAttr(ITEM_ATTRIBUTE_DURATION)));
	}

	ItemDecayState_t decayState = getDecaying();
	if (decayState == DECAYING_TRUE || decayState == DECAYING_PENDING) {
		script.writeText(fmt::format(" DecayState={:d}", tvp::to_underlying(decayState)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_NAME)) {
		script.writeText(fmt::format(" Name=\"{:s}\"", getStrAttr(ITEM_ATTRIBUTE_NAME)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_PLURALNAME)) {
		script.writeText(fmt::format(" PluralName=\"{:s}\"", getStrAttr(ITEM_ATTRIBUTE_PLURALNAME)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_ARTICLE)) {
		script.writeText(fmt::format(" Article=\"{:s}\"", getStrAttr(ITEM_ATTRIBUTE_ARTICLE)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_WEIGHT)) {
		script.writeText(fmt::format(" Weight={:d}", getIntAttr(ITEM_ATTRIBUTE_WEIGHT)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_ATTACK)) {
		script.writeText(fmt::format(" Attack={:d}", getIntAttr(ITEM_ATTRIBUTE_ATTACK)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_ATTACK_SPEED)) {
		script.writeText(fmt::format(" AttackSpeed={:d}", getIntAttr(ITEM_ATTRIBUTE_ATTACK_SPEED)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_DEFENSE)) {
		script.writeText(fmt::format(" Defense={:d}", getIntAttr(ITEM_ATTRIBUTE_DEFENSE)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_EXTRADEFENSE)) {
		script.writeText(fmt::format(" ExtraDefense={:d}", getIntAttr(ITEM_ATTRIBUTE_EXTRADEFENSE)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_ARMOR)) {
		script.writeText(fmt::format(" Armor={:d}", getIntAttr(ITEM_ATTRIBUTE_ARMOR)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_HITCHANCE)) {
		script.writeText(fmt::format(" HitChance={:d}", getIntAttr(ITEM_ATTRIBUTE_HITCHANCE)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_SHOOTRANGE)) {
		script.writeText(fmt::format(" ShootRange={:d}", getIntAttr(ITEM_ATTRIBUTE_SHOOTRANGE)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_DECAYTO)) {
		script.writeText(fmt::format(" DecayTo={:d}", getIntAttr(ITEM_ATTRIBUTE_DECAYTO)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_KEYNUMBER)) {
		script.writeText(fmt::format(" KeyNumber={:d}", getIntAttr(ITEM_ATTRIBUTE_KEYNUMBER)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_KEYHOLENUMBER)) {
		script.writeText(fmt::format(" KeyHoleNumber={:d}", getIntAttr(ITEM_ATTRIBUTE_KEYHOLENUMBER)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_DOORLEVEL)) {
		script.writeText(fmt::format(" DoorLevel={:d}", getIntAttr(ITEM_ATTRIBUTE_DOORLEVEL)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_DOORQUESTNUMBER)) {
		script.writeText(fmt::format(" DoorQuestNumber={:d}", getIntAttr(ITEM_ATTRIBUTE_DOORQUESTNUMBER)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_DOORQUESTVALUE)) {
		script.writeText(fmt::format(" DoorQuestValue={:d}", getIntAttr(ITEM_ATTRIBUTE_DOORQUESTVALUE)));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_CUSTOM)) {
		const ItemAttributes::CustomAttributeMap* customAttrMap = attributes->getCustomAttributeMap();
		for (const auto& entry : *customAttrMap) {
			// Serializing key type and value
			if (entry.second.value.type() == typeid(std::string)) {
				script.writeText(fmt::format(" CustomAttr=({:s}, \"{:s}\")", entry.first,
				                             boost::get<std::string>(entry.second.value)));
			} else if (entry.second.value.type() == typeid(int64_t)) {
				script.writeText(
				    fmt::format(" CustomAttr=({:s}, {:d})", entry.first, boost::get<int64_t>(entry.second.value)));
			} else if (entry.second.value.type() == typeid(bool)) {
				script.writeText(
				    fmt::format(" CustomAttr=({:s}, {:d})", entry.first, boost::get<bool>(entry.second.value)));
			}
		}
	}

	if (const Teleport* teleport = getTeleport()) {
		const auto& destination = teleport->getDestPos();
		script.writeText(fmt::format(" Destination=[{:d},{:d},{:d}]", destination.x, destination.y, destination.z));
	}

	if (const BedItem* bed = getBed()) {
		if (bed->getSleeper()) {
			script.writeText(fmt::format(" Sleeper={:d}", bed->getSleeper()));
		}
	}

	if (const Container* container = getContainer()) {
		if (const DepotLocker* depotLocker = container->getDepotLocker()) {
			script.writeText(fmt::format(" DepotID={:d}", depotLocker->getDepotId()));
		}

		script.writeText(" Content={");
		for (int32_t i = container->getItemList().size() - 1; i >= 0; i--) {
			Item* item = container->getItemByIndex(i);
			item->serializeTVPFormat(script);

			if (i != 0) {
				script.writeText(", ");
			}
		}
		script.writeText("}");
	}
}

Attr_ReadValue Item::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	switch (attr) {
		case ATTR_COUNT:
		case ATTR_RUNE_CHARGES: {
			uint8_t count;
			if (!propStream.read<uint8_t>(count)) {
				return ATTR_READ_ERROR;
			}

			setSubType(count);
			break;
		}

		case ATTR_ACTION_ID: {
			uint16_t actionId;
			if (!propStream.read<uint16_t>(actionId)) {
				return ATTR_READ_ERROR;
			}

			setActionId(actionId);
			break;
		}

		case ATTR_UNIQUE_ID: {
			uint16_t uniqueId;
			if (!propStream.read<uint16_t>(uniqueId)) {
				return ATTR_READ_ERROR;
			}

			setUniqueId(uniqueId);
			break;
		}

		case ATTR_TEXT: {
			std::string text;
			if (!propStream.readString(text)) {
				return ATTR_READ_ERROR;
			}

			setText(text);
			break;
		}

		case ATTR_WRITTENDATE: {
			uint32_t writtenDate;
			if (!propStream.read<uint32_t>(writtenDate)) {
				return ATTR_READ_ERROR;
			}

			setDate(writtenDate);
			break;
		}

		case ATTR_WRITTENBY: {
			std::string writer;
			if (!propStream.readString(writer)) {
				return ATTR_READ_ERROR;
			}

			setWriter(writer);
			break;
		}

		case ATTR_DESC: {
			std::string text;
			if (!propStream.readString(text)) {
				return ATTR_READ_ERROR;
			}

			setSpecialDescription(text);
			break;
		}

		case ATTR_CHARGES: {
			uint16_t charges;
			if (!propStream.read<uint16_t>(charges)) {
				return ATTR_READ_ERROR;
			}

			setSubType(charges);
			break;
		}

		case ATTR_DURATION: {
			int32_t duration;
			if (!propStream.read<int32_t>(duration)) {
				return ATTR_READ_ERROR;
			}

			setDuration(std::max<int32_t>(0, duration));
			break;
		}

		case ATTR_DECAYING_STATE: {
			uint8_t state;
			if (!propStream.read<uint8_t>(state)) {
				return ATTR_READ_ERROR;
			}

			if (state != DECAYING_FALSE) {
				setDecaying(DECAYING_PENDING);
			}
			break;
		}

		case ATTR_NAME: {
			std::string name;
			if (!propStream.readString(name)) {
				return ATTR_READ_ERROR;
			}

			setStrAttr(ITEM_ATTRIBUTE_NAME, name);
			break;
		}

		case ATTR_ARTICLE: {
			std::string article;
			if (!propStream.readString(article)) {
				return ATTR_READ_ERROR;
			}

			setStrAttr(ITEM_ATTRIBUTE_ARTICLE, article);
			break;
		}

		case ATTR_PLURALNAME: {
			std::string pluralName;
			if (!propStream.readString(pluralName)) {
				return ATTR_READ_ERROR;
			}

			setStrAttr(ITEM_ATTRIBUTE_PLURALNAME, pluralName);
			break;
		}

		case ATTR_WEIGHT: {
			uint32_t weight;
			if (!propStream.read<uint32_t>(weight)) {
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_WEIGHT, weight);
			break;
		}

		case ATTR_ATTACK: {
			int32_t attack;
			if (!propStream.read<int32_t>(attack)) {
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_ATTACK, attack);
			break;
		}

		case ATTR_ATTACK_SPEED: {
			uint32_t attackSpeed;
			if (!propStream.read<uint32_t>(attackSpeed)) {
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_ATTACK_SPEED, attackSpeed);
			break;
		}

		case ATTR_DEFENSE: {
			int32_t defense;
			if (!propStream.read<int32_t>(defense)) {
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_DEFENSE, defense);
			break;
		}

		case ATTR_EXTRADEFENSE: {
			int32_t extraDefense;
			if (!propStream.read<int32_t>(extraDefense)) {
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_EXTRADEFENSE, extraDefense);
			break;
		}

		case ATTR_ARMOR: {
			int32_t armor;
			if (!propStream.read<int32_t>(armor)) {
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_ARMOR, armor);
			break;
		}

		case ATTR_HITCHANCE: {
			int8_t hitChance;
			if (!propStream.read<int8_t>(hitChance)) {
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_HITCHANCE, hitChance);
			break;
		}

		case ATTR_SHOOTRANGE: {
			uint8_t shootRange;
			if (!propStream.read<uint8_t>(shootRange)) {
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SHOOTRANGE, shootRange);
			break;
		}

		case ATTR_DECAYTO: {
			int32_t decayTo;
			if (!propStream.read<int32_t>(decayTo)) {
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_DECAYTO, decayTo);
			break;
		}

		case ATTR_KEYNUMBER: {
			uint16_t keyNumber;
			if (!propStream.read<uint16_t>(keyNumber)) {
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_KEYNUMBER, keyNumber);
			break;
		}

		case ATTR_KEYHOLENUMBER: {
			uint16_t keyHoleNumber;
			if (!propStream.read<uint16_t>(keyHoleNumber)) {
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_KEYHOLENUMBER, keyHoleNumber);
			break;
		}

		case ATTR_DOORQUESTNUMBER: {
			uint16_t doorQuestNumber;
			if (!propStream.read<uint16_t>(doorQuestNumber)) {
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_DOORQUESTNUMBER, doorQuestNumber);
			break;
		}

		case ATTR_DOORQUESTVALUE: {
			uint16_t doorQuestValue;
			if (!propStream.read<uint16_t>(doorQuestValue)) {
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_DOORQUESTVALUE, doorQuestValue);
			break;
		}

		case ATTR_DOORLEVEL: {
			uint16_t doorLevel;
			if (!propStream.read<uint16_t>(doorLevel)) {
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_DOORLEVEL, doorLevel);
			break;
		}

		// these should be handled through derived classes
		// If these are called then something has changed in the items.xml since the map was saved
		// just read the values

		// Depot class
		case ATTR_DEPOT_ID: {
			if (!propStream.skip(2)) {
				return ATTR_READ_ERROR;
			}
			break;
		}

		// Door class
		case ATTR_HOUSEDOORID: {
			if (!propStream.skip(1)) {
				return ATTR_READ_ERROR;
			}
			break;
		}

		// Bed class
		case ATTR_SLEEPERGUID: {
			if (!propStream.skip(4)) {
				return ATTR_READ_ERROR;
			}
			break;
		}

		case ATTR_SLEEPSTART: {
			if (!propStream.skip(4)) {
				return ATTR_READ_ERROR;
			}
			break;
		}

		// Teleport class
		case ATTR_TELE_DEST: {
			if (!propStream.skip(5)) {
				return ATTR_READ_ERROR;
			}
			break;
		}

		// Container class
		case ATTR_CONTAINER_ITEMS: {
			return ATTR_READ_ERROR;
		}

		case ATTR_CUSTOM_ATTRIBUTES: {
			uint64_t size;
			if (!propStream.read<uint64_t>(size)) {
				return ATTR_READ_ERROR;
			}

			for (uint64_t i = 0; i < size; i++) {
				// Unserialize key type and value
				std::string key;
				if (!propStream.readString(key)) {
					return ATTR_READ_ERROR;
				};

				// Unserialize value type and value
				ItemAttributes::CustomAttribute val;
				if (!val.unserialize(propStream)) {
					return ATTR_READ_ERROR;
				}

				setCustomAttribute(key, val);
			}
			break;
		}

		default:
			return ATTR_READ_ERROR;
	}

	return ATTR_READ_CONTINUE;
}

bool Item::unserializeAttr(PropStream& propStream)
{
	uint8_t attr_type;
	while (propStream.read<uint8_t>(attr_type) && attr_type != 0) {
		Attr_ReadValue ret = readAttr(static_cast<AttrTypes_t>(attr_type), propStream);
		if (ret == ATTR_READ_ERROR) {
			return false;
		} else if (ret == ATTR_READ_END) {
			return true;
		}
	}
	return true;
}

void Item::serializeTVPFormat(PropWriteStream& propWriteStream) const
{
	propWriteStream.write<uint16_t>(getID());
	propWriteStream.write<uint16_t>(0); // attr begin
	serializeAttr(propWriteStream);
	propWriteStream.write<uint16_t>(0); // attr end

	if (const Container* container = getContainer()) {
		propWriteStream.write<uint32_t>(container->size());
		for (Item* item : container->getItemList()) {
			item->serializeTVPFormat(propWriteStream);
		}
	}
}

bool Item::unserializeTVPFormat(PropStream& propStream)
{
	propStream.skip(2);
	if (!unserializeAttr(propStream)) {
		return false;
	}
	propStream.skip(1);
	// 0 already skipped on unserializeAttr

	if (Container* container = getContainer()) {
		uint32_t totalItems = 0;
		if (!propStream.read<uint32_t>(totalItems)) {
			return false;
		}

		for (uint32_t i = 0; i < totalItems; i++) {
			Item* item = Item::CreateItem(propStream);
			if (!item) {
				return false;
			}

			if (!item->unserializeTVPFormat(propStream)) {
				delete item;
				return false;
			}

			container->addItemBack(item);
		}
	}

	return true;
}

bool Item::unserializeItemNode(OTB::Loader&, const OTB::Node&, PropStream& propStream)
{
	return unserializeAttr(propStream);
}

void Item::serializeAttr(PropWriteStream& propWriteStream) const
{
	const ItemType& it = items[id];
	if (it.stackable || it.isFluidContainer() || it.isSplash()) {
		propWriteStream.write<uint8_t>(ATTR_COUNT);
		propWriteStream.write<uint8_t>(getSubType());
	}

	uint16_t charges = getCharges();
	if (charges != 0) {
		propWriteStream.write<uint8_t>(ATTR_CHARGES);
		propWriteStream.write<uint16_t>(charges);
	}

	uint16_t actionId = getActionId();
	if (actionId != 0) {
		propWriteStream.write<uint8_t>(ATTR_ACTION_ID);
		propWriteStream.write<uint16_t>(actionId);
	}

	const std::string& text = getText();
	if (!text.empty()) {
		propWriteStream.write<uint8_t>(ATTR_TEXT);
		propWriteStream.writeString(text);
	}

	const time_t writtenDate = getDate();
	if (writtenDate != 0) {
		propWriteStream.write<uint8_t>(ATTR_WRITTENDATE);
		propWriteStream.write<uint32_t>(writtenDate);
	}

	const std::string& writer = getWriter();
	if (!writer.empty()) {
		propWriteStream.write<uint8_t>(ATTR_WRITTENBY);
		propWriteStream.writeString(writer);
	}

	const std::string& specialDesc = getSpecialDescription();
	if (!specialDesc.empty()) {
		propWriteStream.write<uint8_t>(ATTR_DESC);
		propWriteStream.writeString(specialDesc);
	}

	if (hasAttribute(ITEM_ATTRIBUTE_DURATION)) {
		propWriteStream.write<uint8_t>(ATTR_DURATION);
		propWriteStream.write<uint32_t>(getIntAttr(ITEM_ATTRIBUTE_DURATION));
	}

	ItemDecayState_t decayState = getDecaying();
	if (decayState == DECAYING_TRUE || decayState == DECAYING_PENDING) {
		propWriteStream.write<uint8_t>(ATTR_DECAYING_STATE);
		propWriteStream.write<uint8_t>(decayState);
	}

	if (hasAttribute(ITEM_ATTRIBUTE_NAME)) {
		propWriteStream.write<uint8_t>(ATTR_NAME);
		propWriteStream.writeString(getStrAttr(ITEM_ATTRIBUTE_NAME));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_ARTICLE)) {
		propWriteStream.write<uint8_t>(ATTR_ARTICLE);
		propWriteStream.writeString(getStrAttr(ITEM_ATTRIBUTE_ARTICLE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_PLURALNAME)) {
		propWriteStream.write<uint8_t>(ATTR_PLURALNAME);
		propWriteStream.writeString(getStrAttr(ITEM_ATTRIBUTE_PLURALNAME));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_WEIGHT)) {
		propWriteStream.write<uint8_t>(ATTR_WEIGHT);
		propWriteStream.write<uint32_t>(getIntAttr(ITEM_ATTRIBUTE_WEIGHT));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_ATTACK)) {
		propWriteStream.write<uint8_t>(ATTR_ATTACK);
		propWriteStream.write<int32_t>(getIntAttr(ITEM_ATTRIBUTE_ATTACK));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_ATTACK_SPEED)) {
		propWriteStream.write<uint8_t>(ATTR_ATTACK_SPEED);
		propWriteStream.write<uint32_t>(getIntAttr(ITEM_ATTRIBUTE_ATTACK_SPEED));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_DEFENSE)) {
		propWriteStream.write<uint8_t>(ATTR_DEFENSE);
		propWriteStream.write<int32_t>(getIntAttr(ITEM_ATTRIBUTE_DEFENSE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_EXTRADEFENSE)) {
		propWriteStream.write<uint8_t>(ATTR_EXTRADEFENSE);
		propWriteStream.write<int32_t>(getIntAttr(ITEM_ATTRIBUTE_EXTRADEFENSE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_ARMOR)) {
		propWriteStream.write<uint8_t>(ATTR_ARMOR);
		propWriteStream.write<int32_t>(getIntAttr(ITEM_ATTRIBUTE_ARMOR));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_HITCHANCE)) {
		propWriteStream.write<uint8_t>(ATTR_HITCHANCE);
		propWriteStream.write<int8_t>(getIntAttr(ITEM_ATTRIBUTE_HITCHANCE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_SHOOTRANGE)) {
		propWriteStream.write<uint8_t>(ATTR_SHOOTRANGE);
		propWriteStream.write<uint8_t>(getIntAttr(ITEM_ATTRIBUTE_SHOOTRANGE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_DECAYTO)) {
		propWriteStream.write<uint8_t>(ATTR_DECAYTO);
		propWriteStream.write<int32_t>(getIntAttr(ITEM_ATTRIBUTE_DECAYTO));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_KEYNUMBER)) {
		propWriteStream.write<uint8_t>(ATTR_KEYNUMBER);
		propWriteStream.write<int16_t>(getIntAttr(ITEM_ATTRIBUTE_KEYNUMBER));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_KEYHOLENUMBER)) {
		propWriteStream.write<uint8_t>(ATTR_KEYHOLENUMBER);
		propWriteStream.write<int16_t>(getIntAttr(ITEM_ATTRIBUTE_KEYHOLENUMBER));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_DOORLEVEL)) {
		propWriteStream.write<uint8_t>(ATTR_DOORLEVEL);
		propWriteStream.write<int16_t>(getIntAttr(ITEM_ATTRIBUTE_DOORLEVEL));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_DOORQUESTNUMBER)) {
		propWriteStream.write<uint8_t>(ATTR_DOORQUESTNUMBER);
		propWriteStream.write<int16_t>(getIntAttr(ITEM_ATTRIBUTE_DOORQUESTNUMBER));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_DOORQUESTVALUE)) {
		propWriteStream.write<uint8_t>(ATTR_DOORQUESTVALUE);
		propWriteStream.write<int16_t>(getIntAttr(ITEM_ATTRIBUTE_DOORQUESTVALUE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_CUSTOM)) {
		const ItemAttributes::CustomAttributeMap* customAttrMap = attributes->getCustomAttributeMap();
		propWriteStream.write<uint8_t>(ATTR_CUSTOM_ATTRIBUTES);
		propWriteStream.write<uint64_t>(static_cast<uint64_t>(customAttrMap->size()));
		for (const auto& entry : *customAttrMap) {
			// Serializing key type and value
			propWriteStream.writeString(entry.first);

			// Serializing value type and value
			entry.second.serialize(propWriteStream);
		}
	}
}

bool Item::hasProperty(ITEMPROPERTY prop) const
{
	const ItemType& it = items[id];
	switch (prop) {
		case CONST_PROP_BLOCKSOLID:
			return it.blockSolid;
		case CONST_PROP_MOVEABLE:
			return it.moveable && !hasAttribute(ITEM_ATTRIBUTE_UNIQUEID);
		case CONST_PROP_HASHEIGHT:
			return it.hasHeight;
		case CONST_PROP_BLOCKPROJECTILE:
			return it.blockProjectile;
		case CONST_PROP_BLOCKPATH:
			return it.blockPathFind;
		case CONST_PROP_ISVERTICAL:
			return it.isVertical;
		case CONST_PROP_ISHORIZONTAL:
			return it.isHorizontal;
		case CONST_PROP_IMMOVABLEBLOCKSOLID:
			return it.blockSolid && (!it.moveable || hasAttribute(ITEM_ATTRIBUTE_UNIQUEID) ||
			                         (getActionId() >= 1000 && getActionId() <= 2000));
		case CONST_PROP_IMMOVABLEBLOCKPATH:
			return it.blockPathFind && (!it.moveable || hasAttribute(ITEM_ATTRIBUTE_UNIQUEID) ||
			                            getActionId() >= 1000 && getActionId() <= 2000);
		case CONST_PROP_IMMOVABLENOFIELDBLOCKPATH:
			return !it.isMagicField() && it.blockPathFind &&
			       (!it.moveable || hasAttribute(ITEM_ATTRIBUTE_UNIQUEID) ||
			        getActionId() >= 1000 && getActionId() <= 2000);
		case CONST_PROP_NOFIELDBLOCKPATH:
			return !it.isMagicField() && it.blockPathFind;
		case CONST_PROP_SUPPORTHANGABLE:
			return it.isHorizontal || it.isVertical;
		case CONST_PROP_SPECIALFIELDBLOCKPATH:
			return it.specialFieldBlockPath;
		default:
			return false;
	}
}

uint32_t Item::getWeight() const
{
	uint32_t weight = getBaseWeight();
	if (isStackable()) {
		return weight * std::max<uint32_t>(1, getItemCount());
	}
	return weight;
}

std::string Item::getDescription(const ItemType& it, int32_t lookDistance, const Item* item /*= nullptr*/,
                                 int32_t subType /*= -1*/, bool addArticle /*= true*/)
{
	std::ostringstream s;
	s << getNameDescription(it, item, subType, addArticle);

	if (item) {
		subType = item->getSubType();
	}

	if (it.isRune()) {
		uint32_t charges = std::max(static_cast<uint32_t>(1),
		                            static_cast<uint32_t>(item == nullptr ? it.charges : item->getCharges()));

		if (it.runeLevel > 0) {
			s << " for level " << it.runeLevel;
		}

		if (it.runeLevel > 0) {
			s << " and";
		}

		s << " for level " << it.runeMagLevel;
		s << ". It's an \"" << it.runeSpellName << "\"-spell (" << charges << "x). ";
	} else if (it.isDoor() && item) {
		if (item->hasAttribute(ITEM_ATTRIBUTE_DOORLEVEL)) {
			s << " for level " << item->getIntAttr(ITEM_ATTRIBUTE_DOORLEVEL);
		}
		s << ".";
	} else if (it.weaponType != WEAPON_NONE) {
		if (it.weaponType != WEAPON_AMMO && it.weaponType != WEAPON_WAND && (it.attack != 0 || it.defense != 0)) {
			s << " (";
			if (item) {
				s << "Atk:" << static_cast<int>(item->getAttack());
			} else {
				s << "Atk:" << static_cast<int>(it.attack);
			}

			if (it.defense != 0) {
				s << " ";
				s << "Def:" << static_cast<int>(it.defense);
			}

			s << ")";
		}
		s << ".";
	} else if (it.armor != 0) {
		if (it.charges > 0) {
			if (subType > 1) {
				s << " that has " << static_cast<int32_t>(subType) << " charges left";
			} else {
				s << " that has " << it.charges << " charge left";
			}
		}

		s << " (Arm:" << it.armor << ").";
	} else if (it.isFluidContainer()) {
		if (item && item->getFluidType() != 0) {
			s << " of " << items[item->getFluidType()].name << ".";
		} else {
			s << ". It is empty.";
		}
	} else if (it.isSplash()) {
		s << " of ";
		if (item && item->getFluidType() != 0) {
			s << items[item->getFluidType()].name;
		} else {
			s << items[1].name;
		}
		s << ".";
	} else if (it.isContainer()) {
		if (!item || item && !(item->getActionId() >= 1000 && item->getActionId() <= 2000))
			s << " (Vol:" << static_cast<int>(it.maxItems) << ").";
		else
			s << '.';
	} else if (it.isKey()) {
		if (item) {
			s << " (Key:" << static_cast<int>(item->getIntAttr(ITEM_ATTRIBUTE_KEYNUMBER)) << ").";
		} else {
			s << " (Key:0).";
		}
	} else if (it.allowDistRead) {
		s << ".";
		s << std::endl;

		if (item && item->getText() != "") {
			if (lookDistance <= 4) {
				const std::string& writer = item->getWriter();
				if (!writer.empty()) {
					s << writer << " wrote";
					time_t date = item->getDate();
					if (date != 0) {
						s << " on " << formatDateShort(date);
					}
					s << ": ";
				} else {
					s << "You read: ";
				}
				s << item->getText();
			} else {
				s << "You are too far away to read it.";
			}
		} else {
			s << "Nothing is written on it.";
		}
	} else if (it.charges > 0) {
		uint32_t charges = (item == nullptr ? it.charges : item->getCharges());
		if (charges > 1) {
			s << " that has " << static_cast<int>(charges) << " charges left.";
		} else {
			s << " that has 1 charge left.";
		}
	} else if (it.showDuration) {
		if (item && item->hasAttribute(ITEM_ATTRIBUTE_DURATION)) {
			const int32_t duration = std::max<int32_t>(1, ((item->getDuration() / 1000) + 59) / 60);

			s << " that has energy for " << duration << " minute" << (duration > 1 ? "s" : "") << " left.";
		} else {
			s << " that is brand-new.";
		}
	} else {
		s << ".";
	}

	if (it.wieldInfo != 0) {
		s << std::endl << "It can only be wielded properly by ";

		if (it.wieldInfo & WIELDINFO_PREMIUM) {
			s << "premium ";
		}

		if (it.wieldInfo & WIELDINFO_VOCREQ) {
			s << it.vocationString;
		} else {
			s << "players";
		}

		if (it.wieldInfo & WIELDINFO_LEVEL) {
			s << " of level " << static_cast<int>(it.minReqLevel) << " or higher";
		}

		if (it.wieldInfo & WIELDINFO_MAGLV) {
			if (it.wieldInfo & WIELDINFO_LEVEL) {
				s << " and";
			} else {
				s << " of";
			}

			s << " magic level " << static_cast<int>(it.minReqMagicLevel) << " or higher";
		}

		s << ".";
	}

	if (lookDistance <= 1 && it.pickupable) {
		double weight = (item == nullptr ? it.weight : item->getWeight());
		if (weight > 0) {
			s << std::endl << getWeightDescription(it, weight);
		}
	}

	if (item && item->getBed() && !item->getText().empty()) {
		s << ' ' << item->getText() << " is sleeping there.";
	} else if (item && item->getSpecialDescription() != "") {
		if (item->getDoor()) {
			s << ' ' << item->getSpecialDescription().c_str();
		} else {
			s << ' ' << item->getSpecialDescription().c_str();
		}
	} else if (it.description.length() && lookDistance <= 1) {
		s << std::endl << it.description;
	}

	return s.str();
}

std::string Item::getDescription(int32_t lookDistance) const
{
	const ItemType& it = items[id];
	return getDescription(it, lookDistance, this);
}

std::string Item::getNameDescription(const ItemType& it, const Item* item /*= nullptr*/, int32_t subType /*= -1*/,
                                     bool addArticle /*= true*/)
{
	if (item) {
		subType = item->getSubType();
	}

	std::ostringstream s;

	const std::string& name = (item ? item->getName() : it.name);
	if (!name.empty()) {
		if (it.stackable && subType > 1 && !it.article.empty()) {
			if (it.showCount) {
				s << subType << ' ';
			}

			if (!it.pluralName.empty() || item && !item->getPluralName().empty()) {
				s << (item ? item->getPluralName() : it.getPluralName());
			} else {
				s << name;
			}
		} else {
			if (addArticle) {
				const std::string& article = (item ? item->getArticle() : it.article);
				if (!article.empty()) {
					s << article << ' ';
				}
			}

			s << name;
		}
	} else {
		if (addArticle) {
			s << "an ";
		}
		s << "item of type " << it.id;
	}
	return s.str();
}

std::string Item::getNameDescription() const
{
	const ItemType& it = items[id];
	return getNameDescription(it, this);
}

std::string Item::getWeightDescription(const ItemType& it, uint32_t weight, uint32_t count /*= 1*/)
{
	std::ostringstream ss;
	if (it.stackable && count > 1 && it.showCount != 0) {
		ss << "They weigh ";
	} else {
		ss << "It weighs ";
	}

	if (weight < 100) {
		ss << "0." << weight;
	} else {
		std::string weightString = std::to_string(weight / 10);
		weightString.insert(weightString.end() - 1, '.');
		ss << weightString;
	}

	ss << " oz.";
	return ss.str();
}

std::string Item::getWeightDescription(uint32_t weight) const
{
	const ItemType& it = Item::items[id];
	return getWeightDescription(it, weight, getItemCount());
}

std::string Item::getWeightDescription() const
{
	uint32_t weight = getWeight();
	if (weight == 0) {
		return std::string();
	}
	return getWeightDescription(weight);
}

void Item::setUniqueId(uint16_t n)
{
	if (hasAttribute(ITEM_ATTRIBUTE_UNIQUEID)) {
		return;
	}

	if (g_game.addUniqueItem(n, this)) {
		getAttributes()->setUniqueId(n);
	}
}

bool Item::canDecay() const
{
	if (isRemoved()) {
		return false;
	}

	const ItemType& it = Item::items[id];
	if (getDecayTo() < 0 || it.decayTime == 0) {
		return false;
	}

	if (hasAttribute(ITEM_ATTRIBUTE_UNIQUEID)) {
		return false;
	}

	if (getActionId() >= 1000 && getActionId() <= 2000) {
		return false;
	}

	return true;
}

uint32_t Item::getWorth() const
{
	switch (id) {
		case ITEM_GOLD_COIN:
			return count;

		case ITEM_PLATINUM_COIN:
			return count * 100;

		case ITEM_CRYSTAL_COIN:
			return count * 10000;

		default:
			return 0;
	}
}

LightInfo Item::getLightInfo() const
{
	const ItemType& it = items[id];
	return {it.lightLevel, it.lightColor};
}

std::string ItemAttributes::emptyString;
int64_t ItemAttributes::emptyInt;
double ItemAttributes::emptyDouble;
bool ItemAttributes::emptyBool;

const std::string& ItemAttributes::getStrAttr(itemAttrTypes type) const
{
	if (!isStrAttrType(type)) {
		return emptyString;
	}

	const Attribute* attr = getExistingAttr(type);
	if (!attr) {
		return emptyString;
	}
	return *attr->value.string;
}

void ItemAttributes::setStrAttr(itemAttrTypes type, const std::string& value)
{
	if (!isStrAttrType(type)) {
		return;
	}

	if (value.empty()) {
		return;
	}

	Attribute& attr = getAttr(type);
	delete attr.value.string;
	attr.value.string = new std::string(value);
}

void ItemAttributes::removeAttribute(itemAttrTypes type)
{
	if (!hasAttribute(type)) {
		return;
	}

	auto prev_it = attributes.rbegin();
	if ((*prev_it).type == type) {
		attributes.pop_back();
	} else {
		auto it = prev_it, end = attributes.rend();
		while (++it != end) {
			if ((*it).type == type) {
				(*it) = attributes.back();
				attributes.pop_back();
				break;
			}
		}
	}
	attributeBits &= ~type;
}

int64_t ItemAttributes::getIntAttr(itemAttrTypes type) const
{
	if (!isIntAttrType(type)) {
		return 0;
	}

	const Attribute* attr = getExistingAttr(type);
	if (!attr) {
		return 0;
	}
	return attr->value.integer;
}

void ItemAttributes::setIntAttr(itemAttrTypes type, int64_t value)
{
	if (!isIntAttrType(type)) {
		return;
	}

	if (type == ITEM_ATTRIBUTE_ATTACK_SPEED && value < 100) {
		value = 100;
	}

	getAttr(type).value.integer = value;
}

void ItemAttributes::increaseIntAttr(itemAttrTypes type, int64_t value) { setIntAttr(type, getIntAttr(type) + value); }

const ItemAttributes::Attribute* ItemAttributes::getExistingAttr(itemAttrTypes type) const
{
	if (hasAttribute(type)) {
		for (const Attribute& attribute : attributes) {
			if (attribute.type == type) {
				return &attribute;
			}
		}
	}
	return nullptr;
}

ItemAttributes::Attribute& ItemAttributes::getAttr(itemAttrTypes type)
{
	if (hasAttribute(type)) {
		for (Attribute& attribute : attributes) {
			if (attribute.type == type) {
				return attribute;
			}
		}
	}

	attributeBits |= type;
	attributes.emplace_back(type);
	return attributes.back();
}

void Item::startDecaying()
{
	if (getActionId() >= 1000 && getActionId() <= 2000) {
		// Quest items should never decay
		return;
	}

	g_game.startDecay(this);
}

bool Item::isHouseItem() const
{
	const ItemType& type = Item::items.getItemType(getID());
	return type.isDoor() || type.moveable || type.forceSerialize || type.isBed() || type.canWriteText ||
	       type.isContainer();
}

template <>
const std::string& ItemAttributes::CustomAttribute::get<std::string>()
{
	if (value.type() == typeid(std::string)) {
		return boost::get<std::string>(value);
	}

	return emptyString;
}

template <>
const int64_t& ItemAttributes::CustomAttribute::get<int64_t>()
{
	if (value.type() == typeid(int64_t)) {
		return boost::get<int64_t>(value);
	}

	return emptyInt;
}

template <>
const double& ItemAttributes::CustomAttribute::get<double>()
{
	if (value.type() == typeid(double)) {
		return boost::get<double>(value);
	}

	return emptyDouble;
}

template <>
const bool& ItemAttributes::CustomAttribute::get<bool>()
{
	if (value.type() == typeid(bool)) {
		return boost::get<bool>(value);
	}

	return emptyBool;
}
