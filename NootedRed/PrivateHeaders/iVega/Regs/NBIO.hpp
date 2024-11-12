// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

//-------- Base Index 0 --------//

constexpr UInt32 mmPCIE_INDEX2 = 0xE;
constexpr UInt32 mmPCIE_DATA2 = 0xF;

//-------- Base Index 2 --------//

constexpr UInt32 mmRCC_DEV0_EPF0_STRAP0 = 0xF;
constexpr UInt8 RCC_DEV0_EPF0_STRAP0_ATI_REV_ID_SHIFT = 0x18;
constexpr UInt32 RCC_DEV0_EPF0_STRAP0_ATI_REV_ID_MASK = 0xF000000;
