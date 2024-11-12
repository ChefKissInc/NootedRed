// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>
#include <PrivateHeaders/GPUDriversAMD/SMU.hpp>

//-------- CAIL Result --------//

enum CAILResult {
    kCAILResultSuccess = 0,
    kCAILResultInvalidArgument,
    kCAILResultFailed,
    kCAILResultUninitialised,
    kCAILResultUnsupported,
};
static_assert(sizeof(CAILResult) == 0x4);

inline CAILResult processSMUFWResponse(const UInt32 value) {
    switch (value) {
        case kSMUFWResponseNoResponse:
            return kCAILResultUninitialised;
        case kSMUFWResponseSuccess:
            return kCAILResultSuccess;
        case kSMUFWResponseRejectedBusy:
            DBGLOG("CAIL", "SMU FW command rejected; SMU is busy");
            return kCAILResultFailed;
        case kSMUFWResponseRejectedPrereq:
            DBGLOG("CAIL", "SMU FW command rejected; prequisite was not satisfied");
            return kCAILResultFailed;
        case kSMUFWResponseUnknownCommand:
            DBGLOG("CAIL", "Unknown SMU FW command");
            return kCAILResultUnsupported;
        case kSMUFWResponseFailed:
            DBGLOG("CAIL", "SMU FW command failed");
            return kCAILResultFailed;
        default:
            SYSLOG("CAIL", "Unknown SMU FW response %d", value);
            return kCAILResultFailed;
    }
}
