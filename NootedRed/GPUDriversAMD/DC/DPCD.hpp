// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

union DPCDSinkExtCaps {
    struct {
        UInt8 sdrAUXBacklightControl : 1;
        UInt8 hdrAUXBacklightControl : 1;
        UInt8 _rsvd : 2;
        UInt8 oled : 1;
        UInt8 _rsvd1 : 3;
    };
    UInt8 raw;
};
