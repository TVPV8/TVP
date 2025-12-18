// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include <random>

#include "position.h"
#include "const.h"
#include "enums.h"

uint32_t PackAbsoluteCoordinate(int x, int y, int z);
uint32_t PackRelativeCoordinate(int x, int y, int z);

uint32_t getSlotPositionBits(int32_t value);

bool folderHasFiles(const char* path, const char* extension);
bool isASCII(const std::string& s);

uint8_t getLiquidColor(uint8_t type);
void printXMLError(const std::string& where, const std::string& fileName, const pugi::xml_parse_result& result);

std::string transformToSHA1(const std::string& input);

void replaceString(std::string& str, const std::string& sought, const std::string& replacement);
void trim_right(std::string& source, char t);
void trim_left(std::string& source, char t);
void toLowerCaseString(std::string& source);
std::string asLowerCaseString(std::string source);
std::string asUpperCaseString(std::string source);

using StringVector = std::vector<std::string>;
using IntegerVector = std::vector<int32_t>;

StringVector explodeString(const std::string& inString, const std::string& separator, int32_t limit = -1);
IntegerVector vectorAtoi(const StringVector& stringVector);
constexpr bool hasBitSet(uint32_t flag, uint32_t flags) {
	return (flags & flag) != 0;
}

std::mt19937& getRandomGenerator();
int32_t random(int32_t minNumber, int32_t maxNumber);
int32_t uniform_random(int32_t minNumber, int32_t maxNumber);
int32_t normal_random(int32_t minNumber, int32_t maxNumber);
bool boolean_random(double probability = 0.5);

Direction getDirection(const std::string& string);
Position getNextPosition(Direction direction, Position pos);
Direction getDirectionTo(const Position& from, const Position& to);

std::string getFirstLine(const std::string& str);

std::string formatDate(time_t time);
std::string formatDateShort(time_t time);
std::string convertIPToString(uint32_t ip);

int32_t countSpaces(const std::string& str);
bool compareSpellWords(const std::string& spellWords, std::string givenWords, bool supportParam = false);
std::string mergeSpellWords(const std::string& words);
std::string removeExtraSpaces(const std::string& str);

void trimString(std::string& str);

MagicEffectClasses getMagicEffect(const std::string& strValue);
ShootType_t getShootType(const std::string& strValue);
Ammo_t getAmmoType(const std::string& strValue);
WeaponAction_t getWeaponAction(const std::string& strValue);
Skulls_t getSkullType(const std::string& strValue);
std::string getCombatName(CombatType_t combatType);

std::string getSpecialSkillName(uint8_t skillid);
std::string getSkillName(uint8_t skillid);

std::string ucfirst(std::string str);
std::string ucwords(std::string str);
bool booleanString(const std::string& str);

std::string getWeaponName(WeaponType_t weaponType);

size_t combatTypeToIndex(CombatType_t combatType);
CombatType_t indexToCombatType(size_t v);

itemAttrTypes stringToItemAttribute(const std::string& str);

const char* getReturnMessage(ReturnValue value);

int64_t OTSYS_TIME();

namespace tvp {

#if __has_cpp_attribute(__cpp_lib_to_underlying)

inline constexpr auto to_underlying(auto e) noexcept { return std::to_underlying(e); }

#else

inline constexpr auto to_underlying(auto e) noexcept { return static_cast<std::underlying_type_t<decltype(e)>>(e); }

#endif

} // namespace tvp
