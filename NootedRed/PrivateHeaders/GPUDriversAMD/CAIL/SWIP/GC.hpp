// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

enum GCFirmwareType {
    kGCFirmwareTypeRLCSRListCntl = 0,
    kGCFirmwareTypeRLCSRListGPMMem,
    kGCFirmwareTypeRLCSRListSRMMem,
    kGCFirmwareTypeRLC,
    kGCFirmwareTypeME,
    kGCFirmwareTypeCE,
    kGCFirmwareTypePFP,
    kGCFirmwareTypeMEC1,
    kGCFirmwareTypeMEC2,
    kGCFirmwareTypeMECJT1,
    kGCFirmwareTypeMECJT2,
    kGCFirmwareTypeRLCV,
    kGCFirmwareTypeRLCP,
    kGCFirmwareTypeRLCLX6IRAM,
    kGCFirmwareTypeRLCLX6DRAM,
    kGCFirmwareTypeRLCTOC,
    kGCFirmwareTypeGlobalTapDelays,
    kGCFirmwareTypeSE0TapDelays,
    kGCFirmwareTypeSE1TapDelays,
    kGCFirmwareTypeSE2TapDelays,
    kGCFirmwareTypeSE3TapDelays,
    kGCFirmwareTypeCount,
};

struct GCFirmwareConstant {
    const char *version;
    UInt32 field8;
    UInt32 romSize;
    UInt32 actualPayloadOffDWords;
    UInt32 payloadSizeDWords;
    UInt32 field18;
    UInt32 field1C;
    const UInt8 *rom;
    UInt32 checksum;
    UInt16 field2C;
    UInt16 field2E;
    UInt32 payloadOffDWords;
};

struct GCFirmwareInfo {
    UInt32 count;
    const GCFirmwareConstant *entry[kGCFirmwareTypeCount];
    void *handle[kGCFirmwareTypeCount];
};

struct GCFirmwareEntry {
    bool valid;
    GCFirmwareType id;
    const void *rom;
    UInt32 romSize;
    UInt32 payloadOff;
    void *handle;
    UInt32 version;
    UInt32 field24;
    UInt32 field28;
    UInt32 field2C;
};
