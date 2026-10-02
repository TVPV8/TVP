// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include <boost/lexical_cast.hpp>

namespace pugi {
template <typename T>
T cast(const pugi::char_t* str)
{
	T value;
	try {
		value = boost::lexical_cast<T>(str);
	} catch (boost::bad_lexical_cast&) {
		value = T();
	}
	return value;
}
} // namespace pugi
