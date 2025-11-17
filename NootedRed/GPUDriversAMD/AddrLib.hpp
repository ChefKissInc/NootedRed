// AMD AddrLib
//
// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
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

static constexpr UInt32 ADDR_CHIP_FAMILY_AI = 8;

static constexpr UInt32 Dcn1NonBpp64SwModeMask = 0x2220221;
static constexpr UInt32 Dcn1Bpp64SwModeMask = 0x6660661;
static constexpr UInt32 Dcn2NonBpp64SwModeMask = 0x2020201;
static constexpr UInt32 Dcn2Bpp64SwModeMask = 0x6060601;

static constexpr UInt32 Dcn1NonBpp64SwModeMask1015 = 0x22222221;
static constexpr UInt32 Dcn1Bpp64SwModeMask1015 = 0x66666661;
static constexpr UInt32 Dcn2NonBpp64SwModeMask1015 = 0x22022201;
static constexpr UInt32 Dcn2Bpp64SwModeMask1015 = 0x66066601;

union ADDR2_SURFACE_FLAGS {
    struct {
        UInt32 color : 1;
        UInt32 depth : 1;
        UInt32 stencil : 1;
        UInt32 fmask : 1;
        UInt32 overlay : 1;
        UInt32 display : 1;
        UInt32 prt : 1;
        UInt32 qbStereo : 1;
        UInt32 interleaved : 1;
        UInt32 texture : 1;
        UInt32 unordered : 1;
        UInt32 rotated : 1;
        UInt32 needEquation : 1;
        UInt32 opt4space : 1;
        UInt32 minimizeAlign : 1;
        UInt32 noMetadata : 1;
        UInt32 metaRbUnaligned : 1;
        UInt32 metaPipeUnaligned : 1;
        UInt32 view3dAs2dArray : 1;
        UInt32 reserved : 13;
    };

    UInt32 value;

    constexpr ADDR2_SURFACE_FLAGS() : value {0} {}
};

enum AddrResourceType {
    ADDR_RSRC_TEX_1D = 0,
    ADDR_RSRC_TEX_2D = 1,
    ADDR_RSRC_TEX_3D = 2,
    ADDR_RSRC_MAX_TYPE = 3,
};

struct ADDR2_COMPUTE_SURFACE_INFO_INPUT {
    UInt32 size {sizeof(ADDR2_COMPUTE_SURFACE_INFO_INPUT)};
    ADDR2_SURFACE_FLAGS flags {};
    UInt32 swizzleMode {0};
    AddrResourceType resourceType {};
    UInt32 format {0};
    UInt32 bpp {0};
    UInt32 width {0};
    UInt32 height {0};
    UInt32 numSlices {0};
    UInt32 numMipLevels {0};
    UInt32 numSamples {0};
    UInt32 numFrags {0};
    UInt32 pitchInElement {0};
    UInt32 sliceAlign {0};
};
static_assert(sizeof(ADDR2_COMPUTE_SURFACE_INFO_INPUT) == 0x38);

struct ADDR2_COMPUTE_SURFACE_INFO_OUTPUT {
    UInt32 size {sizeof(ADDR2_COMPUTE_SURFACE_INFO_OUTPUT)};
    UInt32 pitch {0};
    UInt32 height {0};
    UInt32 numSlices {0};
    UInt32 mipChainPitch {0};
    UInt32 mipChainHeight {0};
    UInt32 mipChainSlice {0};
    UInt64 sliceSize {0};
    UInt64 surfSize {0};
    UInt32 baseAlign {0};
    UInt32 bpp {0};
    UInt32 pixelMipChainPitch {0};
    UInt32 pixelMipChainHeight {0};
    UInt32 pixelPitch {0};
    UInt32 pixelHeight {0};
    UInt32 pixelBits {0};
    UInt32 blockWidth {0};
    UInt32 blockHeight {0};
    UInt32 blockSlices {0};
    UInt32 epitchIsHeight {0};
    void *pStereoInfo {nullptr};
    void *pMipInfo {nullptr};
    UInt32 equationIndex {0};
    UInt32 mipChainInTail {0};
    UInt32 firstMipIdInTail {0};
};
static_assert(sizeof(ADDR2_COMPUTE_SURFACE_INFO_OUTPUT) == 0x80);
