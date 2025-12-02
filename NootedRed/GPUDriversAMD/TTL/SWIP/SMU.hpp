// AMD TTL SWIP SMU
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once

enum AMDSWIPSMUEvent {
    SMU_EVENT_POWER_UP = 0,
    SMU_EVENT_POWER_DOWN,
    SMU_EVENT_RESET = 9,
    SMU_EVENT_REINITIALISE,
    SMU_EVENT_COLLECT_DEBUG_INFO,
    SMU_EVENT_COUNT,
};
