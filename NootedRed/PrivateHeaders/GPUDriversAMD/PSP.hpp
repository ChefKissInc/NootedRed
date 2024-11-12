// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

enum AMDPSPCommand {
    kPSPCommandLoadTA = 1,
    kPSPCommandLoadASD = 4,
    kPSPCommandLoadIPFW = 6,
};
static_assert(sizeof(AMDPSPCommand) == 0x4);

enum AMDUCodeID {
    kUCodeCE = 2,
    kUCodePFP,
    kUCodeME,
    kUCodeMEC1JT,
    kUCodeMEC2JT,
    kUCodeMEC1,
    kUCodeMEC2,
    kUCodeRLC = 11,
    kUCodeSDMA0,
    kUCodeDMCUERAM = 19,
    kUCodeDMCUISR,
    kUCodeRLCV = 21,
    kUCodeRLCSRListGPM = 23,
    kUCodeRLCSRListSRM,
    kUCodeRLCSRListCntl,
    kUCodeDMCUB = 35,
};
static_assert(sizeof(AMDUCodeID) == 0x4);
