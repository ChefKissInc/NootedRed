// Version-independent interface to the `AMDRadeonX5000_AMDHWAlignManager` class
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/AddrLib.hpp>
#include <Headers/kern_util.hpp>

struct AMDTilingInfo {
    UInt32 tilingConfig;
    UInt32 field_4;
    UInt32 field_8;
    UInt32 field_c;
    UInt32 field_10;
    UInt32 field_14;
    UInt32 field_18;
};

enum struct ATIPixelMode : UInt8 {
    Invalid,
    C_8,
    C_16,
    C_8_8,
    C_32,
    C_16_16,
    C_10_11_11,
    C_11_11_10,
    C_10_10_10_2,
    C_2_10_10_10,
    C_8_8_8_8,
    C_32_32,
    C_16_16_16_16,
    Reserved_13,
    C_32_32_32_32,
    Reserved_15,
    C_5_6_5,
    C_1_5_5_5,
    C_5_5_5_1,
    C_4_4_4_4,
    C_8_24,
    C_24_8,
    C_X24_8_32_FLOAT,
    Reserved_23,
    HW_DEFINED = 0xFF,
};
static_assert(sizeof(ATIPixelMode) == 0x1);

inline const char *stringifyATIPixelMode(ATIPixelMode v) {
    switch (v) {
        case ATIPixelMode::Invalid:
            return "Invalid";
        case ATIPixelMode::C_8:
            return "C_8";
        case ATIPixelMode::C_16:
            return "C_16";
        case ATIPixelMode::C_8_8:
            return "C_8_8";
        case ATIPixelMode::C_32:
            return "C_32";
        case ATIPixelMode::C_16_16:
            return "C_16_16";
        case ATIPixelMode::C_10_11_11:
            return "C_10_11_11";
        case ATIPixelMode::C_11_11_10:
            return "C_11_11_10";
        case ATIPixelMode::C_10_10_10_2:
            return "C_10_10_10_2";
        case ATIPixelMode::C_2_10_10_10:
            return "C_2_10_10_10";
        case ATIPixelMode::C_8_8_8_8:
            return "C_8_8_8_8";
        case ATIPixelMode::C_32_32:
            return "C_32_32";
        case ATIPixelMode::C_16_16_16_16:
            return "C_16_16_16_16";
        case ATIPixelMode::Reserved_13:
            return "Reserved_13";
        case ATIPixelMode::C_32_32_32_32:
            return "C_32_32_32_32";
        case ATIPixelMode::Reserved_15:
            return "Reserved_15";
        case ATIPixelMode::C_5_6_5:
            return "C_5_6_5";
        case ATIPixelMode::C_1_5_5_5:
            return "C_1_5_5_5";
        case ATIPixelMode::C_5_5_5_1:
            return "C_5_5_5_1";
        case ATIPixelMode::C_4_4_4_4:
            return "C_4_4_4_4";
        case ATIPixelMode::C_8_24:
            return "C_8_24";
        case ATIPixelMode::C_24_8:
            return "C_24_8";
        case ATIPixelMode::C_X24_8_32_FLOAT:
            return "C_X24_8_32_FLOAT";
        case ATIPixelMode::Reserved_23:
            return "Reserved_23";
        case ATIPixelMode::HW_DEFINED:
            return "HW_DEFINED";
    }
}

enum struct ATIFormat : UInt8 {
    Null,
    Luminance8,
    Alpha8,
    Intensity8,
    LuminanceAlpha8,
    RGB565,
    BGR565,
    ARGB4,
    BGRA4,
    ABGR4,
    RGBA4,
    ARGB1555,
    ABGR1555,
    RGBA5551,
    BGRA5551,
    BGRA8,
    RGBA8,
    ABGR8,
    ARGB8,
    ARGB8Noswap,
    RGB4S3tc,
    RGBADxt3,
    RGBADxt5,
    LA3DC,
    Red3DC,
    ARGB16161616F = Red3DC,
    SignedRed3DC,
    RG3DC,
    SignedRG3DC,
    Depth16,
    BGRA32F,
    RGBA32F,
    RGBA32U,
    Luminance32F,
    Alpha32F,
    Intensity32F,
    LuminanceAlpha32F,
    BGRA16,
    RGBA16,
    Luminance16,
    Alpha16,
    Intensity16,
    LuminanceAlpha16,
    BGRA16F,
    RGBA16F,
    Luminance16F,
    Alpha16F,
    Intensity16F,
    LuminanceAlpha16F,
    ABGR2101010,
    ARGB2101010,
    RGBA1010102,
    BGRA1010102,
    AVYU444,
    VYUY422,
    YVYU422,
    BGRG422,
    GBGR422,
    RGB332,
    R8,
    R16,
    Rg8,
    Rg16,
    R16F,
    R32F,
    Rg16F,
    Rg32F,
    Depth24Stencil8,
    Depth32,
    Depth32F,
    Depth32fStencil8,
    BGRA32U,
    Alpha32U,
    Intensity32U,
    Luminance32U,
    LuminanceAlpha32U,
    RGBA16U,
    BGRA16U,
    Alpha16U,
    Intensity16U,
    Luminance16U,
    LuminanceAlpha16U,
    ARGB2101010U,
    ABGR2101010U,
    RGBA1010102U,
    BGRA1010102U,
    ARGB8UNoSwap,
    ABGR8U,
    RGBA8U,
    BGRA8U,
    Alpha8U,
    Intensity8U,
    Luminance8U,
    LuminanceAlpha8U,
    RGBA5551U,
    BGRA5551U,
    BGRA4U,
    ABGR4U,
    BGR565U,
    RGB332U,
    R8U,
    R16U,
    R32U,
    Rg8U,
    Rg16U,
    Rg32U,
    RGBA32I,
    BGRA32I,
    Alpha32I,
    Intensity32I,
    Luminance32I,
    LuminanceAlpha32I,
    RGBA16I,
    BGRA16I,
    Alpha16I,
    Intensity16I,
    Luminance16I,
    LuminanceAlpha16I,
    RGBA8I,
    BGRA8I,
    Alpha8I,
    Intensity8I,
    Luminance8I,
    LuminanceAlpha8I,
    R8I,
    R16I,
    R32I,
    RG8I,
    RG16I,
    RG32I,
    R11G11B10Float,
    R9G9B9E5SharedExp,
    R8Snorm,
    Rg8Snorm,
    RGBA8Snorm,
    BGRA8Snorm,
    R16Snorm,
    Rg16Snorm,
    RGBA16Snorm,
    BGRA16Snorm,
    Stencil8,
    RGBBPTCSignedF,
    RGBBPTCUnsignedF,
    RGBABPTCUnorm,
    BGRX8,
    DvdYuv2,
    DvdYuv9,
    DvdYuv12,
    DvdDecode,
    DvdLastDecode,
    None,
};
static_assert(sizeof(ATIFormat) == 0x1);

inline const char *stringifyATIFormat(ATIFormat v) {
    switch (v) {
        case ATIFormat::Null:
            return "Null";
        case ATIFormat::Luminance8:
            return "Luminance8";
        case ATIFormat::Alpha8:
            return "Alpha8";
        case ATIFormat::Intensity8:
            return "Intensity8";
        case ATIFormat::LuminanceAlpha8:
            return "LuminanceAlpha8";
        case ATIFormat::RGB565:
            return "RGB565";
        case ATIFormat::BGR565:
            return "BGR565";
        case ATIFormat::ARGB4:
            return "ARGB4";
        case ATIFormat::BGRA4:
            return "BGRA4";
        case ATIFormat::ABGR4:
            return "ABGR4";
        case ATIFormat::RGBA4:
            return "RGBA4";
        case ATIFormat::ARGB1555:
            return "ARGB1555";
        case ATIFormat::ABGR1555:
            return "ABGR1555";
        case ATIFormat::RGBA5551:
            return "RGBA5551";
        case ATIFormat::BGRA5551:
            return "BGRA5551";
        case ATIFormat::BGRA8:
            return "BGRA8";
        case ATIFormat::RGBA8:
            return "RGBA8";
        case ATIFormat::ABGR8:
            return "ABGR8";
        case ATIFormat::ARGB8:
            return "ARGB8";
        case ATIFormat::ARGB8Noswap:
            return "ARGB8Noswap";
        case ATIFormat::RGB4S3tc:
            return "RGB4S3tc";
        case ATIFormat::RGBADxt3:
            return "RGBADxt3";
        case ATIFormat::RGBADxt5:
            return "RGBADxt5";
        case ATIFormat::LA3DC:
            return "LA3DC";
        case ATIFormat::Red3DC:
            return "Red3DC";
        case ATIFormat::SignedRed3DC:
            return "SignedRed3DC";
        case ATIFormat::RG3DC:
            return "RG3DC";
        case ATIFormat::SignedRG3DC:
            return "SignedRG3DC";
        case ATIFormat::Depth16:
            return "Depth16";
        case ATIFormat::BGRA32F:
            return "BGRA32F";
        case ATIFormat::RGBA32F:
            return "RGBA32F";
        case ATIFormat::RGBA32U:
            return "RGBA32U";
        case ATIFormat::Luminance32F:
            return "Luminance32F";
        case ATIFormat::Alpha32F:
            return "Alpha32F";
        case ATIFormat::Intensity32F:
            return "Intensity32F";
        case ATIFormat::LuminanceAlpha32F:
            return "LuminanceAlpha32F";
        case ATIFormat::BGRA16:
            return "BGRA16";
        case ATIFormat::RGBA16:
            return "RGBA16";
        case ATIFormat::Luminance16:
            return "Luminance16";
        case ATIFormat::Alpha16:
            return "Alpha16";
        case ATIFormat::Intensity16:
            return "Intensity16";
        case ATIFormat::LuminanceAlpha16:
            return "LuminanceAlpha16";
        case ATIFormat::BGRA16F:
            return "BGRA16F";
        case ATIFormat::RGBA16F:
            return "RGBA16F";
        case ATIFormat::Luminance16F:
            return "Luminance16F";
        case ATIFormat::Alpha16F:
            return "Alpha16F";
        case ATIFormat::Intensity16F:
            return "Intensity16F";
        case ATIFormat::LuminanceAlpha16F:
            return "LuminanceAlpha16F";
        case ATIFormat::ABGR2101010:
            return "ABGR2101010";
        case ATIFormat::ARGB2101010:
            return "ARGB2101010";
        case ATIFormat::RGBA1010102:
            return "RGBA1010102";
        case ATIFormat::BGRA1010102:
            return "BGRA1010102";
        case ATIFormat::AVYU444:
            return "AVYU444";
        case ATIFormat::VYUY422:
            return "VYUY422";
        case ATIFormat::YVYU422:
            return "YVYU422";
        case ATIFormat::BGRG422:
            return "BGRG422";
        case ATIFormat::GBGR422:
            return "GBGR422";
        case ATIFormat::RGB332:
            return "RGB332";
        case ATIFormat::R8:
            return "R8";
        case ATIFormat::R16:
            return "R16";
        case ATIFormat::Rg8:
            return "Rg8";
        case ATIFormat::Rg16:
            return "Rg16";
        case ATIFormat::R16F:
            return "R16F";
        case ATIFormat::R32F:
            return "R32F";
        case ATIFormat::Rg16F:
            return "Rg16F";
        case ATIFormat::Rg32F:
            return "Rg32F";
        case ATIFormat::Depth24Stencil8:
            return "Depth24Stencil8";
        case ATIFormat::Depth32:
            return "Depth32";
        case ATIFormat::Depth32F:
            return "Depth32F";
        case ATIFormat::Depth32fStencil8:
            return "Depth32fStencil8";
        case ATIFormat::BGRA32U:
            return "BGRA32U";
        case ATIFormat::Alpha32U:
            return "Alpha32U";
        case ATIFormat::Intensity32U:
            return "Intensity32U";
        case ATIFormat::Luminance32U:
            return "Luminance32U";
        case ATIFormat::LuminanceAlpha32U:
            return "LuminanceAlpha32U";
        case ATIFormat::RGBA16U:
            return "RGBA16U";
        case ATIFormat::BGRA16U:
            return "BGRA16U";
        case ATIFormat::Alpha16U:
            return "Alpha16U";
        case ATIFormat::Intensity16U:
            return "Intensity16U";
        case ATIFormat::Luminance16U:
            return "Luminance16U";
        case ATIFormat::LuminanceAlpha16U:
            return "LuminanceAlpha16U";
        case ATIFormat::ARGB2101010U:
            return "ARGB2101010U";
        case ATIFormat::ABGR2101010U:
            return "ABGR2101010U";
        case ATIFormat::RGBA1010102U:
            return "RGBA1010102U";
        case ATIFormat::BGRA1010102U:
            return "BGRA1010102U";
        case ATIFormat::ARGB8UNoSwap:
            return "ARGB8UNoSwap";
        case ATIFormat::ABGR8U:
            return "ABGR8U";
        case ATIFormat::RGBA8U:
            return "RGBA8U";
        case ATIFormat::BGRA8U:
            return "BGRA8U";
        case ATIFormat::Alpha8U:
            return "Alpha8U";
        case ATIFormat::Intensity8U:
            return "Intensity8U";
        case ATIFormat::Luminance8U:
            return "Luminance8U";
        case ATIFormat::LuminanceAlpha8U:
            return "LuminanceAlpha8U";
        case ATIFormat::RGBA5551U:
            return "RGBA5551U";
        case ATIFormat::BGRA5551U:
            return "BGRA5551U";
        case ATIFormat::BGRA4U:
            return "BGRA4U";
        case ATIFormat::ABGR4U:
            return "ABGR4U";
        case ATIFormat::BGR565U:
            return "BGR565U";
        case ATIFormat::RGB332U:
            return "RGB332U";
        case ATIFormat::R8U:
            return "R8U";
        case ATIFormat::R16U:
            return "R16U";
        case ATIFormat::R32U:
            return "R32U";
        case ATIFormat::Rg8U:
            return "Rg8U";
        case ATIFormat::Rg16U:
            return "Rg16U";
        case ATIFormat::Rg32U:
            return "Rg32U";
        case ATIFormat::RGBA32I:
            return "RGBA32I";
        case ATIFormat::BGRA32I:
            return "BGRA32I";
        case ATIFormat::Alpha32I:
            return "Alpha32I";
        case ATIFormat::Intensity32I:
            return "Intensity32I";
        case ATIFormat::Luminance32I:
            return "Luminance32I";
        case ATIFormat::LuminanceAlpha32I:
            return "LuminanceAlpha32I";
        case ATIFormat::RGBA16I:
            return "RGBA16I";
        case ATIFormat::BGRA16I:
            return "BGRA16I";
        case ATIFormat::Alpha16I:
            return "Alpha16I";
        case ATIFormat::Intensity16I:
            return "Intensity16I";
        case ATIFormat::Luminance16I:
            return "Luminance16I";
        case ATIFormat::LuminanceAlpha16I:
            return "LuminanceAlpha16I";
        case ATIFormat::RGBA8I:
            return "RGBA8I";
        case ATIFormat::BGRA8I:
            return "BGRA8I";
        case ATIFormat::Alpha8I:
            return "Alpha8I";
        case ATIFormat::Intensity8I:
            return "Intensity8I";
        case ATIFormat::Luminance8I:
            return "Luminance8I";
        case ATIFormat::LuminanceAlpha8I:
            return "LuminanceAlpha8I";
        case ATIFormat::R8I:
            return "R8I";
        case ATIFormat::R16I:
            return "R16I";
        case ATIFormat::R32I:
            return "R32I";
        case ATIFormat::RG8I:
            return "RG8I";
        case ATIFormat::RG16I:
            return "RG16I";
        case ATIFormat::RG32I:
            return "RG32I";
        case ATIFormat::R11G11B10Float:
            return "R11G11B10Float";
        case ATIFormat::R9G9B9E5SharedExp:
            return "R9G9B9E5SharedExp";
        case ATIFormat::R8Snorm:
            return "R8Snorm";
        case ATIFormat::Rg8Snorm:
            return "Rg8Snorm";
        case ATIFormat::RGBA8Snorm:
            return "RGBA8Snorm";
        case ATIFormat::BGRA8Snorm:
            return "BGRA8Snorm";
        case ATIFormat::R16Snorm:
            return "R16Snorm";
        case ATIFormat::Rg16Snorm:
            return "Rg16Snorm";
        case ATIFormat::RGBA16Snorm:
            return "RGBA16Snorm";
        case ATIFormat::BGRA16Snorm:
            return "BGRA16Snorm";
        case ATIFormat::Stencil8:
            return "Stencil8";
        case ATIFormat::RGBBPTCSignedF:
            return "RGBBPTCSignedF";
        case ATIFormat::RGBBPTCUnsignedF:
            return "RGBBPTCUnsignedF";
        case ATIFormat::RGBABPTCUnorm:
            return "RGBABPTCUnorm";
        case ATIFormat::BGRX8:
            return "BGRX8";
        case ATIFormat::DvdYuv2:
            return "DvdYuv2";
        case ATIFormat::DvdYuv9:
            return "DvdYuv9";
        case ATIFormat::DvdYuv12:
            return "DvdYuv12";
        case ATIFormat::DvdDecode:
            return "DvdDecode";
        case ATIFormat::DvdLastDecode:
            return "DvdLastDecode";
        case ATIFormat::None:
            return "None";
    }
}

class AMDRadeonX5000_AMDHWAlignManager {
    public:
    auto getAddrFormat(const ATIPixelMode pixelMode) {
        auto vtable = getMember<void *>(this, 0);
        return getMember<UInt32 (*)(AMDRadeonX5000_AMDHWAlignManager *, ATIPixelMode)>(vtable, 0x1C8)(this, pixelMode);
    }

    IOReturn getSurfaceInfo2(ADDR2_COMPUTE_SURFACE_INFO_INPUT *const input,
        ADDR2_COMPUTE_SURFACE_INFO_OUTPUT *const output) {
        auto vtable = getMember<void *>(this, 0);
        return getMember<IOReturn (*)(AMDRadeonX5000_AMDHWAlignManager *, ADDR2_COMPUTE_SURFACE_INFO_INPUT *,
            ADDR2_COMPUTE_SURFACE_INFO_OUTPUT *)>(vtable, 0x130)(this, input, output);
    }
};
