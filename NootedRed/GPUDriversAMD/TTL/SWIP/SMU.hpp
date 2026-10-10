// AMD TTL SWIP SMU
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

struct AMDSMUFWConstants
{
    bool   valid;
    UInt32 version;
    void*  ptr;
    UInt32 length;
    void*  memHandle;
};

struct AMDSMUSWInitOutput
{
    UInt32            structSize;
    void*             context;
    AMDSMUFWConstants fwConstants;
};

struct AMDSMUUCodeConstants
{
    UInt32 version;
    UInt32 length;
    void*  ptr;
};
