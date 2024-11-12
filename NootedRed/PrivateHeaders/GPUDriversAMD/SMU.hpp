// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

enum AMDSMUFWResponse {
    kSMUFWResponseNoResponse = 0,
    kSMUFWResponseSuccess = 1,
    kSMUFWResponseRejectedBusy = 0xFC,
    kSMUFWResponseRejectedPrereq = 0xFD,
    kSMUFWResponseUnknownCommand = 0xFE,
    kSMUFWResponseFailed = 0xFF,
};
static_assert(sizeof(AMDSMUFWResponse) == 0x4);
