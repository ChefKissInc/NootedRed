// AMD TTL Events
//
// Copyright (C) 2025-2026  ChefKiss.
//
// Licensed under the ChefKiss Open Knowledge License version 1.
//
// This work is licensed to you under the terms of the ChefKiss Open Knowledge
// License, either version 1 of the License, or (at your option) any later
// version published by ChefKiss.
//
// This work may not be used to train or develop artificial intelligence,
// and may not be used with artificial intelligence in any other way. See
// the ChefKiss Open Knowledge License for details.
//
// This work is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// ChefKiss Open Knowledge License for more details.
//
// You should have received a copy of the ChefKiss Open Knowledge License
// along with this work.  If not, see https://git.chefkiss.dev/ChefKiss/OpenKnowledge/raw/branch/master/LICENSE-v1.md.

#pragma once
#include <IOKit/IOTypes.h>

enum TTLEvent
{
    TTL_EVENT_POWER_UP = 1,
    TTL_EVENT_POWER_DOWN,
    TTL_EVENT_RESET = 10,
    TTL_EVENT_FULL_RESET,
    TTL_EVENT_HW_INIT_COMPLETE,
    TTL_EVENT_HW_UNINIT_COMPLETE,
    TTL_EVENT_COLLECT_DEBUG_INFO,
};

struct TTLEventInput
{
    UInt32 id;
    UInt32 arg;
};

enum TTLFullScreenEvent
{
    TTL_FULLSCREEN_EVENT_INCREASE = 1,
    TTL_FULLSCREEN_EVENT_RESET    = 2,
};
