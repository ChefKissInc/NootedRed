// AMD TTL SWIP SMU
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/CAIL/Result.hpp>
#include <GPUDriversAMD/SMU.hpp>
#include <Headers/kern_util.hpp>
#include <IOKit/IOTypes.h>

inline CAILResult processSMUFWResponse(const UInt32 value) {
    switch (value) {
        case kSMUFWResponseNoResponse:
            return kCAILResultNoResponse;
        case kSMUFWResponseSuccess:
            return kCAILResultOK;
        case kSMUFWResponseRejectedBusy:
            SYSTRACE("CAIL", "SMU FW command rejected; SMU is busy");
            return kCAILResultInvalidParameters;
        case kSMUFWResponseRejectedPrereq:
            SYSTRACE("CAIL", "SMU FW command rejected; prerequisite was not satisfied");
            return kCAILResultInvalidParameters;
        case kSMUFWResponseUnknownCommand:
            SYSTRACE("CAIL", "Unknown SMU FW command");
            return kCAILResultUnsupported;
        case kSMUFWResponseFailed:
            SYSTRACE("CAIL", "SMU FW command failed");
            return kCAILResultInvalidParameters;
        default:
            SYSTRACE("CAIL", "Unknown SMU FW response %d", value);
            return kCAILResultInvalidParameters;
    }
}

enum AMDSWIPSMUEvent {
    SMU_EVENT_POWER_UP = 0,
    SMU_EVENT_POWER_DOWN,
    SMU_EVENT_RESET = 9,
    SMU_EVENT_REINITIALISE,
    SMU_EVENT_COLLECT_DEBUG_INFO,
    SMU_EVENT_COUNT,
};
