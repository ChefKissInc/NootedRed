// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

struct CommonFirmwareHeader {
    UInt32 size;
    UInt32 headerSize;
    UInt16 headerMajor;
    UInt16 headerMinor;
    UInt16 ipMajor;
    UInt16 ipMinor;
    UInt32 ucodeVer;
    UInt32 ucodeSize;
    UInt32 ucodeOff;
    UInt32 crc32;
};

struct GPUInfoFirmware {
    UInt32 gcNumSe;
    UInt32 gcNumCuPerSh;
    UInt32 gcNumShPerSe;
    UInt32 gcNumRbPerSe;
    UInt32 gcNumTccs;
    UInt32 gcNumGprs;
    UInt32 gcNumMaxGsThds;
    UInt32 gcGsTableDepth;
    UInt32 gcGsPrimBuffDepth;
    UInt32 gcParameterCacheDepth;
    UInt32 gcDoubleOffchipLdsBuffer;
    UInt32 gcWaveSize;
    UInt32 gcMaxWavesPerSimd;
    UInt32 gcMaxScratchSlotsPerCu;
    UInt32 gcLdsSize;
};
