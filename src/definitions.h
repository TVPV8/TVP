// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

static constexpr auto STATUS_SERVER_NAME = "The Violet Project";
static constexpr auto STATUS_SERVER_VERSION = "3.1 Beta";
static constexpr auto STATUS_SERVER_DEVELOPERS = "Ezzz";

static constexpr auto CLIENT_VERSION_MIN = 772;
static constexpr auto CLIENT_VERSION_MAX = 772;
static constexpr auto CLIENT_VERSION_STR = "7.72";

#define BOOST_ASIO_NO_DEPRECATED
#define OPENSSL_NO_DEPRECATED

#ifndef __FUNCTION__
#define __FUNCTION__ __func__
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif

#define WIN32_LEAN_AND_MEAN

#ifdef _MSC_VER
#ifdef NDEBUG
#define _SECURE_SCL 0
#define HAS_ITERATOR_DEBUGGING 0
#endif

#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4244) // 'argument' : conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable : 4250) // 'class1' : inherits 'class2::member' via dominance
#pragma warning(disable : 4267) // 'var' : conversion from 'size_t' to 'type', possible loss of data
#pragma warning(disable : 4319) // '~': zero extending 'unsigned int' to 'lua_Number' of greater size
#pragma warning(disable : 4351) // new behavior: elements of array will be default initialized
#pragma warning(disable : 4458) // declaration hides class member
#pragma warning(disable : 4242) // possible loss of data
#endif

#define strcasecmp _stricmp
#define strncasecmp _strnicmp

#ifndef _WIN32_WINNT
// 0x0602: Windows 7
#define _WIN32_WINNT 0x0602
#endif
#endif
