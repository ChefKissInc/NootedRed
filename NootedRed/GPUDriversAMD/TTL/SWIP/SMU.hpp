// AMD TTL SWIP SMU
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/CAIL/Result.hpp>
#include <GPUDriversAMD/SMU.hpp>
#include <Headers/kern_util.hpp>
#include <IOKit/IOTypes.h>

inline CAILResult processSMUFWResponse(const UInt32 msg, const UInt32 value)
{
    switch (value) {
        case kSMUFWResponseNoResponse: return kCAILResultNoResponse;
        case kSMUFWResponseSuccess   : return kCAILResultOK;
        case kSMUFWResponseRejectedBusy:
            SYSLOG("CAIL", "SMU FW command 0x%X rejected; SMU is busy", msg);
            return kCAILResultInvalidParameters;
        case kSMUFWResponseRejectedPrereq:
            SYSLOG("CAIL", "SMU FW command 0x%X rejected; prerequisite was not satisfied", msg);
            return kCAILResultInvalidParameters;
        case kSMUFWResponseUnknownCommand:
            SYSLOG("CAIL", "Unknown SMU FW command 0x%X", msg);
            return kCAILResultUnsupported;
        case kSMUFWResponseFailed:
            SYSLOG("CAIL", "SMU FW command 0x%X failed", msg);
            return kCAILResultInvalidParameters;
        default:
            SYSLOG("CAIL", "Unknown SMU FW response %d for command 0x%X", value, msg);
            return kCAILResultInvalidParameters;
    }
}

enum AMDSWIPSMUEvent
{
    SMU_EVENT_POWER_UP = 0,
    SMU_EVENT_POWER_DOWN,
    SMU_EVENT_RESET = 9,
    SMU_EVENT_REINITIALISE,
    SMU_EVENT_COLLECT_DEBUG_INFO,
    SMU_EVENT_COUNT,
};
