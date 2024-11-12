// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

struct Gfx9ChipSettings {
    UInt32 isArcticIsland : 1;
    UInt32 isVega10 : 1;
    UInt32 isRaven : 1;
    UInt32 isVega12 : 1;
    UInt32 isVega20 : 1;
    UInt32 reserved0 : 27;
    UInt32 isDce12 : 1;
    UInt32 isDcn1 : 1;
    UInt32 reserved1 : 30;
    UInt32 metaBaseAlignFix : 1;
    UInt32 depthPipeXorDisable : 1;
    UInt32 htileAlignFix : 1;
    UInt32 applyAliasFix : 1;
    UInt32 htileCacheRbConflict : 1;
    UInt32 reserved2 : 27;
};

constexpr UInt32 ADDR_CHIP_FAMILY_AI = 8;

constexpr UInt32 Dcn1NonBpp64SwModeMask = 0x2220221;
constexpr UInt32 Dcn1Bpp64SwModeMask = 0x6660661;
constexpr UInt32 Dcn2NonBpp64SwModeMask = 0x2020201;
constexpr UInt32 Dcn2Bpp64SwModeMask = 0x6060601;

constexpr UInt32 Dcn1NonBpp64SwModeMask1015 = 0x22222221;
constexpr UInt32 Dcn1Bpp64SwModeMask1015 = 0x66666661;
constexpr UInt32 Dcn2NonBpp64SwModeMask1015 = 0x22022201;
constexpr UInt32 Dcn2Bpp64SwModeMask1015 = 0x66066601;
