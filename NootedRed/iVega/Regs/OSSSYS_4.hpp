// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

constexpr UInt32 IH_CLK_CTRL = 0x117B;
constexpr UInt32 IH_IH_BUFFER_MEM_CLK_SOFT_OVERRIDE_SHIFT = 0x1A;
constexpr UInt32 IH_DBUS_MUX_CLK_SOFT_OVERRIDE_SHIFT = 0x1B;
constexpr UInt32 IH_CHICKEN = 0x122C;
constexpr UInt32 IH_MC_SPACE_GPA_ENABLE = getBit(4);
