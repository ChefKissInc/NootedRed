// AMD TTL Events
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

enum TTLEvent {
    TTL_EVENT_POWER_UP = 1,
    TTL_EVENT_POWER_DOWN,
    TTL_EVENT_RESET = 10,
    TTL_EVENT_FULL_RESET,
    TTL_EVENT_HW_INIT_COMPLETE,
    TTL_EVENT_HW_UNINIT_COMPLETE,
    TTL_EVENT_COLLECT_DEBUG_INFO,
};

struct TTLEventInput {
    UInt32 id;
    UInt32 arg;
};

enum TTLFullScreenEvent {
    TTL_FULLSCREEN_EVENT_INCREASE = 1,
    TTL_FULLSCREEN_EVENT_RESET = 2,
};
