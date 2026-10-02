// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

namespace xtea {

using key = std::array<uint32_t, 4>;
using round_keys = std::array<uint32_t, 64>;

round_keys expand_key(const key& k);
void encrypt(uint8_t* data, size_t length, const round_keys& k);
void decrypt(uint8_t* data, size_t length, const round_keys& k);

} // namespace xtea
