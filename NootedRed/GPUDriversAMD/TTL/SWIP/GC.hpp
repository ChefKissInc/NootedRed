// AMD TTL SWIP Graphics and Compute
//
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
    const void *rom;
    UInt32 checksum;
    UInt16 field2C;
    UInt16 field2E;
    UInt32 payloadOffDWords;
};

#define GC_FW_CONSTANT(_V, _F8, _APO, _PSD, _F18, _F1C, _R, _C, _F2C, _F2E, _PO)                                    \
    static const GCFirmwareConstant _R {                                                                            \
        .version = _V, .field8 = _F8, .romSize = sizeof(_##_R), .actualPayloadOffDWords = _APO,                     \
        .payloadSizeDWords = _PSD, .field18 = _F18, .field1C = _F1C, .rom = _##_R, .checksum = _C, .field2C = _F2C, \
        .field2E = _F2E, .payloadOffDWords = _PO,                                                                   \
    }

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
