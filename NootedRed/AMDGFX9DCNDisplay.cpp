// DCN Display implementation for GFX9
// Derivative of AMDRadeonX5000 and AMDRadeonX6000 decompilation
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <AMDGFX9DCNDisplay.hpp>
#include <GPUDriversAMD/Accel/HWAlignManager.hpp>
#include <GPUDriversAMD/Accel/HWDisplay.hpp>
#include <GPUDriversAMD/AddrLib.hpp>
#include <GPUDriversAMD/FB/Attributes.hpp>
#include <GPUDriversAMD/FB/FramebufferInfo.hpp>
#include <GPUDriversAMD/Packet3.hpp>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>
#include <IOKit/IOLib.h>
#include <IOKit/IOReturn.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/graphics/IOGraphicsTypes.h>
#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/RuntimeMC.hpp>
#include <PenguinWizardry/RuntimeVFT.hpp>
#include <libkern/OSTypes.h>
#include <libkern/c++/OSData.h>
#include <libkern/c++/OSMetaClass.h>
#include <mach/i386/vm_types.h>
#include <mach/vm_param.h>

PWDefineAbstractRuntimeMCWithExpansion(AMDRadeonX5000_AMDGFX9DCNDisplay, Expansion)

#define GET_GB_ADDR_CONFIG_NUM_PIPES(v)            ((v) & 7)
#define GET_GB_ADDR_CONFIG_PIPE_INTERLEAVE(v)      (((v) >> 3) & 7)
#define GET_GB_ADDR_CONFIG_MAX_COMPRESSED_FRAGS(v) (((v) >> 6) & 3)
#define GET_GB_ADDR_CONFIG_NUM_BANKS(v)            (((v) >> 12) & 7)
#define GET_GB_ADDR_CONFIG_NUM_SE(v)               (((v) >> 19) & 3)
#define GET_GB_ADDR_CONFIG_NUM_RB_PER_SE(v)        (((v) >> 26) & 3)

#define SURFACE_CONFIG_PIXEL_FORMAT(v)   ((v) & 0x7F)
#define SURFACE_CONFIG_ROTATION_ANGLE(v) (((v) & 3) << 8)

#define HUBP_ADDR_CONFIG_NUM_PIPES(v)            ((v) & 7)
#define HUBP_ADDR_CONFIG_NUM_BANKS(v)            (((v) & 7) << 3)
#define HUBP_ADDR_CONFIG_PIPE_INTERLEAVE(v)      (((v) & 3) << 6)
#define HUBP_ADDR_CONFIG_NUM_SE(v)               (((v) & 3) << 8)
#define HUBP_ADDR_CONFIG_NUM_RB_PER_SE(v)        (((v) & 3) << 10)
#define HUBP_ADDR_CONFIG_MAX_COMPRESSED_FRAGS(v) (((v) & 3) << 12)

#define TILING_CONFIG_SW_MODE(v)  ((v) & 31)
#define TILING_CONFIG_DIM_TYPE(v) (((v) & 3) << 7)

static UInt32 HUBPREQ_FLIP_CONTROL_FLIP_TYPE(bool v) { return static_cast<UInt32>(v) << 1; }
static void   SET_HUBPREQ_FLIP_CONTROL_FLIP_TYPE(UInt32& target, bool v)
{
    target &= ~getBit<UInt32>(1);
    target |= static_cast<UInt32>(v) << 1;
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::initDCNRegOffs()
{ vft().getExpanded<void(AMDRadeonX5000_AMDGFX9DCNDisplay*)>(this, 0)(this); }

void AMDRadeonX5000_AMDGFX9DCNDisplay::initialiseRegisters(AMDRadeonX5000_AMDHWDisplay* const _self)
{
    const auto self      = static_cast<AMDRadeonX5000_AMDGFX9DCNDisplay*>(_self);
    auto&      expansion = self->getExpansion();
    for (size_t i = 0; i < self->supportedDisplayCount(); i++) {
        auto&       savedState = expansion.savedState[i];
        const auto& regOffs    = expansion.regOffs[i];
        assert(regOffs.isValid);
        savedState.hubpreqflipControl = self->getHWRegisters()->read(regOffs.hubpreqFlipControl);
    }
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::restoreRegisters(AMDRadeonX5000_AMDHWDisplay*) { }

static bool (*superGetDisplayInfo)(AMDRadeonX5000_AMDHWDisplay* self, UInt32 fbIndex, bool isCRTEnabled,
                                   bool ignoreCRTOffsetCheck, IOFramebuffer* fb, FramebufferInfo* fbInfo) = nullptr;

bool AMDRadeonX5000_AMDGFX9DCNDisplay::getDisplayInfo(AMDRadeonX5000_AMDHWDisplay* const _self, const UInt32 fbIndex,
                                                      const bool isCRTEnabled, const bool ignoreCRTOffsetCheck,
                                                      IOFramebuffer* const fb, FramebufferInfo* const fbInfo)
{
    const auto self = static_cast<AMDRadeonX5000_AMDGFX9DCNDisplay*>(_self);
    assert(superGetDisplayInfo != nullptr);
    if (!superGetDisplayInfo(self, fbIndex, isCRTEnabled, ignoreCRTOffsetCheck, fb, fbInfo)) { return false; }

    if (!isCRTEnabled) { return true; }

    auto&       expansion    = self->getExpansion();
    auto&       savedState   = expansion.savedState[fbIndex];
    const auto& displayState = self->displayStates()[fbIndex];

    memset(&savedState.flipParam, 0, sizeof(savedState.flipParam));

    AMDHWRotationAngle hwRotation = AMDHWRotationAngle::DEG_0;
    if (!displayState.status.isAccelBacked()) {
        const auto rotateFlags = self->scalerFlags()[fbIndex] & kIOScaleRotateFlags;
        switch (rotateFlags) {
            case kIOScaleRotate0: {
                hwRotation = AMDHWRotationAngle::DEG_0;
            } break;
            case kIOScaleRotate90: {
                hwRotation = AMDHWRotationAngle::DEG_90;
            } break;
            case kIOScaleRotate180: {
                hwRotation = AMDHWRotationAngle::DEG_180;
            } break;
            case kIOScaleRotate270: {
                hwRotation = AMDHWRotationAngle::DEG_270;
            } break;
            default: return false;
        }
    }

    UInt32                  hwPixelFormat;
    AMDHWSurfacePixelFormat pixelFormat;
    switch (displayState.pixelInfo.bitsPerPixel) {
        case 16: {
            hwPixelFormat = 1;
            pixelFormat   = AMDHWSurfacePixelFormat::ARGB1555;
        } break;
        case 32: {
            if (displayState.pixelInfo.bitsPerComponent == 10) {
                hwPixelFormat = 10;
                pixelFormat   = AMDHWSurfacePixelFormat::ARGB2101010;
            }
            else {
                hwPixelFormat = 8;
                pixelFormat   = AMDHWSurfacePixelFormat::ARGB8888;
            }
        } break;
        case 64: {
            hwPixelFormat = 24;
            pixelFormat   = AMDHWSurfacePixelFormat::ARGB16161616F;
        } break;
        default: return false;
    }

    const auto addrConfig = self->getHWInterface()->getAddrConfig();
    savedState.hubpSurfaceConfig =
        SURFACE_CONFIG_PIXEL_FORMAT(hwPixelFormat) | SURFACE_CONFIG_ROTATION_ANGLE(static_cast<UInt32>(hwRotation));
    savedState.hubpControl = 0;
    savedState.hubpAddrConfig =
        HUBP_ADDR_CONFIG_NUM_PIPES(GET_GB_ADDR_CONFIG_NUM_PIPES(addrConfig))
        | HUBP_ADDR_CONFIG_NUM_BANKS(GET_GB_ADDR_CONFIG_NUM_BANKS(addrConfig))
        | HUBP_ADDR_CONFIG_PIPE_INTERLEAVE(GET_GB_ADDR_CONFIG_PIPE_INTERLEAVE(addrConfig))
        | HUBP_ADDR_CONFIG_NUM_SE(GET_GB_ADDR_CONFIG_NUM_SE(addrConfig))
        | HUBP_ADDR_CONFIG_NUM_RB_PER_SE(GET_GB_ADDR_CONFIG_NUM_RB_PER_SE(addrConfig))
        | HUBP_ADDR_CONFIG_MAX_COMPRESSED_FRAGS(GET_GB_ADDR_CONFIG_MAX_COMPRESSED_FRAGS(addrConfig));
    savedState.hubpTilingConfig = TILING_CONFIG_SW_MODE(self->swizzleModes()[fbIndex]) | TILING_CONFIG_DIM_TYPE(1);
    savedState.hwRotation       = hwRotation;
    savedState.flipParam.dcn.surfaceFormat   = pixelFormat;
    savedState.flipParam.dcn.surfaceRotation = hwRotation;
    self->fillFlipTilingParameters(&savedState.flipParam, self->swizzleModes()[fbIndex]);

    return true;
}

UInt64 AMDRadeonX5000_AMDGFX9DCNDisplay::getCurrentDisplayOffset(AMDRadeonX5000_AMDHWDisplay* const _self,
                                                                 const UInt32                       fbIndex)
{
    const auto  self      = static_cast<AMDRadeonX5000_AMDGFX9DCNDisplay*>(_self);
    auto&       expansion = self->getExpansion();
    const auto& regOffs   = expansion.regOffs[fbIndex];
    assert(regOffs.isValid);

    return static_cast<UInt64>(self->getHWRegisters()->read(regOffs.hubpreqPrimarySurfaceAddressHigh)) << 32
           | self->getHWRegisters()->read(regOffs.hubpreqPrimarySurfaceAddress);
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::setCurrentDisplayOffset(AMDRadeonX5000_AMDHWDisplay* const _self,
                                                               const UInt32 fbIndex, const UInt64 value)
{
    const auto  self      = static_cast<AMDRadeonX5000_AMDGFX9DCNDisplay*>(_self);
    auto&       expansion = self->getExpansion();
    const auto& regOffs   = expansion.regOffs[fbIndex];
    assert(regOffs.isValid);
    assert(expansion.regShiftsMasks.isValid);
    self->setFlipControlRegister(self, fbIndex, AMDSwapInterval::Immediate);
    self->getHWRegisters()->write(regOffs.hubpreqPrimarySurfaceAddressHigh,
                                  (value >> 32) & expansion.regShiftsMasks.primarySurfaceHi);
    self->getHWRegisters()->write(regOffs.hubpreqPrimarySurfaceAddress, value & 0xFFFFFFFF);
    expansion.lastSubmitFlipOffset = value;
    while (self->isFlipPending(self, fbIndex)) { IODelay(100); }
}

UInt32 AMDRadeonX5000_AMDGFX9DCNDisplay::writeWaitForVLine(AMDRadeonX5000_AMDHWDisplay*, UInt32* const, const UInt32,
                                                           SInt32&, SInt32&, const bool, const bool)
{ PANIC("AMDGFX9DCNDisplay", "%s should not be called!", __func__); }

void AMDRadeonX5000_AMDGFX9DCNDisplay::setFlipControlRegister(AMDRadeonX5000_AMDHWDisplay* const _self,
                                                              const UInt32 fbIndex, const AMDSwapInterval swapInterval)
{
    const auto  self      = static_cast<AMDRadeonX5000_AMDGFX9DCNDisplay*>(_self);
    auto&       expansion = self->getExpansion();
    const auto& regOffs   = expansion.regOffs[fbIndex];
    assert(regOffs.isValid);
    auto& savedState = expansion.savedState[fbIndex];
    SET_HUBPREQ_FLIP_CONTROL_FLIP_TYPE(savedState.hubpreqflipControl, swapInterval == AMDSwapInterval::Immediate);
    self->getHWRegisters()->write(regOffs.hubpreqFlipControl, savedState.hubpreqflipControl);
}

bool AMDRadeonX5000_AMDGFX9DCNDisplay::init(AMDRadeonX5000_AMDHWDisplay* const _self, void* const hwInterface,
                                            void* const fbParams)
{
    const auto self = static_cast<AMDRadeonX5000_AMDGFX9DCNDisplay*>(_self);
    if (!_self->init(hwInterface, fbParams)) { return false; }

    self->isDCN() = true;

    self->initDCNRegOffs();

    return true;
}

// TODO: maybe handle more of these?
ATIPixelMode AMDRadeonX5000_AMDGFX9DCNDisplay::getPixelMode(AMDRadeonX5000_AMDHWDisplay*, const CRTHWDepth depth,
                                                            const CRTHWFormat format)
{
    DBGLOG("GFX9DCNDisplay", "%s << (depth: %s format: %s)", __func__, stringifyCRTHWDepth(depth),
           stringifyCRTHWFormat(format));
    if (depth == CRTHWDepth::DEPTH_8 && format == CRTHWFormat::FORMAT_8) { return ATIPixelMode::C_8; }
    if (depth == CRTHWDepth::DEPTH_16) {
        if (format == CRTHWFormat::FORMAT_8) { return ATIPixelMode::C_1_5_5_5; }
        if (format == CRTHWFormat::FORMAT_10) { return ATIPixelMode::C_5_6_5; }
        if (format == CRTHWFormat::FORMAT_12) { return ATIPixelMode::C_4_4_4_4; }
    }
    if (depth == CRTHWDepth::DEPTH_32) {
        if (format == CRTHWFormat::FORMAT_8) { return ATIPixelMode::C_8_8_8_8; }
        if (format == CRTHWFormat::FORMAT_10) { return ATIPixelMode::C_2_10_10_10; }
    }
    if (depth == CRTHWDepth::DEPTH_64 && format == CRTHWFormat::FORMAT_8) { return ATIPixelMode::C_16_16_16_16; }
    return ATIPixelMode::HW_DEFINED;
}

ATIFormat AMDRadeonX5000_AMDGFX9DCNDisplay::getPixelFormat(AMDRadeonX5000_AMDHWDisplay*, const ATIPixelMode pixelMode)
{
    DBGLOG("GFX9DCNDisplay", "%s << (pixelMode: %s)", __func__, stringifyATIPixelMode(pixelMode));
    ATIFormat ret;
    switch (pixelMode) {
        case ATIPixelMode::C_8          : ret = ATIFormat::BGRA8; break;
        case ATIPixelMode::C_2_10_10_10 : ret = ATIFormat::ARGB2101010; break;
        case ATIPixelMode::C_16_16_16_16: ret = ATIFormat::BGRA16; break;
        case ATIPixelMode::C_5_6_5      :
        case ATIPixelMode::C_1_5_5_5    : ret = ATIFormat::ARGB1555; break;
        default                         : ret = ATIFormat::Luminance8; break;
    }
    DBGLOG("GFX9DCNDisplay", "%s >> %s", __func__, stringifyATIFormat(ret));
    return ret;
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::fillFlipTilingParameters(AMDFlipParam* const flipParam, const UInt32 swizzleMode)
{
    const auto addrConfig                             = this->getHWInterface()->getAddrConfig();
    flipParam->dcn.tilingParams.dcn2.numPipes         = GET_GB_ADDR_CONFIG_NUM_PIPES(addrConfig);
    flipParam->dcn.tilingParams.dcn2.numBanks         = GET_GB_ADDR_CONFIG_NUM_BANKS(addrConfig);
    flipParam->dcn.tilingParams.dcn2.pipeInter        = GET_GB_ADDR_CONFIG_PIPE_INTERLEAVE(addrConfig);
    flipParam->dcn.tilingParams.dcn2.numShaderEngines = GET_GB_ADDR_CONFIG_NUM_SE(addrConfig);
    flipParam->dcn.tilingParams.dcn2.numRbPerSe       = GET_GB_ADDR_CONFIG_NUM_RB_PER_SE(addrConfig);
    flipParam->dcn.tilingParams.dcn2.maxFrags         = getBit(GET_GB_ADDR_CONFIG_MAX_COMPRESSED_FRAGS(addrConfig));
    flipParam->dcn.tilingParams.dcn2.swizzleMode      = swizzleMode;
}

bool AMDRadeonX5000_AMDGFX9DCNDisplay::writeFlipParameters(
    AMDRadeonX5000_AMDHWDisplay* const _self, AMDPipeFlip* const flip, const UInt32 fbIndex, const UInt64 offsetLeft,
    [[maybe_unused]] const UInt64 offsetRight, const AMDSwapInterval swapInterval, const UInt32 pitch,
    const AMDTilingInfo* const tileInfo, const ATIFormat atiFormat, const UInt32, AMDHWRotationAngle* const hwRotation,
    AMDPipeFlip* const savedFlip)
{
    const auto self = static_cast<AMDRadeonX5000_AMDGFX9DCNDisplay*>(_self);
    assert(offsetRight == 0);

    auto& expansion  = self->getExpansion();
    auto& savedState = expansion.savedState[fbIndex];

    AMDFlipParam newFlipParam;
    memset(&newFlipParam, 0, sizeof(newFlipParam));

    switch (atiFormat) {
        case ATIFormat::RGB565: {
            newFlipParam.dcn.surfaceFormat = AMDHWSurfacePixelFormat::RGB565;
        } break;
        case ATIFormat::BGR565: {
            newFlipParam.dcn.surfaceFormat = AMDHWSurfacePixelFormat::BGR565;
        } break;
        case ATIFormat::ARGB1555: {
            newFlipParam.dcn.surfaceFormat = AMDHWSurfacePixelFormat::ARGB1555;
        } break;
        case ATIFormat::ABGR1555: {
            newFlipParam.dcn.surfaceFormat = AMDHWSurfacePixelFormat::ABGR1555;
        } break;
        case ATIFormat::BGRA8: {
            newFlipParam.dcn.surfaceFormat = AMDHWSurfacePixelFormat::ARGB8888;
        } break;
        case ATIFormat::RGBA8: {
            newFlipParam.dcn.surfaceFormat = AMDHWSurfacePixelFormat::ABGRA8888;
        } break;
        case ATIFormat::RGBA16F: {
            newFlipParam.dcn.surfaceFormat = AMDHWSurfacePixelFormat::ABGR16161616F;
        } break;
        case ATIFormat::ABGR2101010: {
            newFlipParam.dcn.surfaceFormat = AMDHWSurfacePixelFormat::ABGR2101010;
        } break;
        case ATIFormat::ARGB2101010: {
            newFlipParam.dcn.surfaceFormat = AMDHWSurfacePixelFormat::ARGB2101010;
        } break;
        case ATIFormat::RGBA1010102: {
            newFlipParam.dcn.surfaceFormat = AMDHWSurfacePixelFormat::RGBA1010102;
        } break;
        case ATIFormat::BGRA1010102: {
            newFlipParam.dcn.surfaceFormat = AMDHWSurfacePixelFormat::BGRA1010102;
        } break;
        default: return false;
    }

    if (hwRotation == nullptr) { newFlipParam.dcn.surfaceRotation = savedState.flipParam.dcn.surfaceRotation; }
    else {
        switch (*hwRotation) {
            case AMDHWRotationAngle::DEG_0:
            case AMDHWRotationAngle::DEG_90:
            case AMDHWRotationAngle::DEG_180:
            case AMDHWRotationAngle::DEG_270: {
                newFlipParam.dcn.surfaceRotation = *hwRotation;
            } break;
            case AMDHWRotationAngle::Undefined: {
                newFlipParam.dcn.surfaceRotation = savedState.flipParam.dcn.surfaceRotation;
            } break;
            default: return false;
        }
    }

    switch (swapInterval) {
        case AMDSwapInterval::Immediate: {
            newFlipParam.dcn.flipType = AMDHWSurfaceFlipType::Immediate;
        } break;
        case AMDSwapInterval::VSync: {
            newFlipParam.dcn.flipType = AMDHWSurfaceFlipType::VSync;
        } break;
        default: {
            newFlipParam.dcn.flipType = AMDHWSurfaceFlipType::HSync;
        } break;
    }

    if (tileInfo == nullptr) {
        newFlipParam.dcn.surfaceFormat     = savedState.flipParam.dcn.surfaceFormat;
        newFlipParam.dcn.surfaceRotation   = savedState.flipParam.dcn.surfaceRotation;
        newFlipParam.dcn.tilingParams.dcn2 = savedState.flipParam.dcn.tilingParams.dcn2;
    }
    else {
        self->fillFlipTilingParameters(&newFlipParam, tileInfo->tilingConfig & 0x1F);
    }

    newFlipParam.dcn.surfacePitch = pitch;
    newFlipParam.dcn.surfaceAddr  = offsetLeft;

    if (savedFlip != nullptr && swapInterval != AMDSwapInterval::VSync) {
        if (savedFlip->flipParam.dcn.surfaceFormat != newFlipParam.dcn.surfaceFormat
            || savedFlip->flipParam.dcn.surfaceRotation != newFlipParam.dcn.surfaceRotation
            || savedFlip->flipParam.dcn.surfacePitch != newFlipParam.dcn.surfacePitch
            || savedFlip->flipParam.dcn.tilingParams.dcn2 != newFlipParam.dcn.tilingParams.dcn2)
        {
            return false;
        }
    }

    if (flip != nullptr) { flip->flipParam = newFlipParam; }
    if (savedFlip != nullptr) { savedFlip->flipParam = newFlipParam; }

    return true;
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::getDisplayModeViewportSpecificInfo(AMDRadeonX5000_AMDHWDisplay* const _self,
                                                                          const UInt32                       fbIndex,
                                                                          UInt32* const viewportYStart,
                                                                          UInt32* const viewportHeight)
{
    const auto  self           = static_cast<AMDRadeonX5000_AMDGFX9DCNDisplay*>(_self);
    auto&       expansion      = self->getExpansion();
    const auto& regOffs        = expansion.regOffs[fbIndex];
    const auto& regShiftsMasks = expansion.regShiftsMasks;
    assert(regOffs.isValid);
    assert(regShiftsMasks.isValid);
    if (viewportYStart != nullptr) {
        *viewportYStart =
            (self->getHWRegisters()->read(regOffs.hubpPriViewportStart) & regShiftsMasks.viewportYStartMask)
            >> regShiftsMasks.viewportYStartShift;
    }
    if (viewportHeight != nullptr) {
        *viewportHeight =
            (self->getHWRegisters()->read(regOffs.hubpPriViewportDimension) & regShiftsMasks.viewportHeightMask)
            >> regShiftsMasks.viewportHeightShift;
    }
}

UInt32 AMDRadeonX5000_AMDGFX9DCNDisplay::writeFlipControlRegisters(AMDRadeonX5000_AMDHWDisplay* const _self,
                                                                   const UInt32 fbIndex, UInt32* const buffer,
                                                                   const AMDSwapInterval         swapInterval,
                                                                   const UInt64                  offsetLeft,
                                                                   [[maybe_unused]] const UInt64 offsetRight)
{
    const auto self = static_cast<AMDRadeonX5000_AMDGFX9DCNDisplay*>(_self);
    assert(offsetRight == 0);

    UInt32 displays[MAX_SUPPORTED_DISPLAYS];
    displays[0]             = fbIndex;
    const auto displayCount = self->getMirroredDisplays(fbIndex, &displays[1]) + 1;
    assert(displayCount <= MAX_SUPPORTED_DISPLAYS_RV);
    const auto& expansion      = self->getExpansion();
    const auto  flipControl    = HUBPREQ_FLIP_CONTROL_FLIP_TYPE(swapInterval == AMDSwapInterval::Immediate);
    UInt32      dwordCount     = 0;
    const auto& regShiftsMasks = expansion.regShiftsMasks;
    assert(regShiftsMasks.isValid);
    for (UInt32 i = 0; i < displayCount; ++i) {
        const auto  displayIndex = displays[i];
        const auto& regOffs      = expansion.regOffs[displayIndex];
        assert(regOffs.isValid);
        const auto& savedState  = expansion.savedState[displayIndex];
        dwordCount             += write1RegWritePacket(&buffer[dwordCount], regOffs.hubpreqFlipControl, flipControl);
        dwordCount +=
            write1RegWritePacket(&buffer[dwordCount], regOffs.hubpSurfaceConfig, savedState.hubpSurfaceConfig);
        dwordCount += write1RegWritePacket(&buffer[dwordCount], regOffs.hubpretControl, savedState.hubpControl);
        dwordCount += write1RegWritePacket(&buffer[dwordCount], regOffs.hubpAddrConfig, savedState.hubpAddrConfig);
        dwordCount += write1RegWritePacket(&buffer[dwordCount], regOffs.hubpTilingConfig, savedState.hubpTilingConfig);
        dwordCount += write1RegWritePacket(&buffer[dwordCount], regOffs.hubpreqPrimarySurfaceAddressHigh,
                                           (offsetLeft >> 32) & regShiftsMasks.primarySurfaceHi);
        dwordCount +=
            write1RegWritePacket(&buffer[dwordCount], regOffs.hubpreqPrimarySurfaceAddress, offsetLeft & 0xFFFFFFFF);
    }

    return dwordCount;
}

bool AMDRadeonX5000_AMDGFX9DCNDisplay::isDisplayControlEnabled(AMDRadeonX5000_AMDHWDisplay* const _self,
                                                               const UInt32                       fbIndex)
{
    const auto  self      = static_cast<AMDRadeonX5000_AMDGFX9DCNDisplay*>(_self);
    const auto& expansion = self->getExpansion();
    const auto& regOffs   = expansion.regOffs[fbIndex];
    assert(regOffs.isValid);
    assert(expansion.regShiftsMasks.isValid);
    return (self->getHWRegisters()->read(regOffs.otgControl) & expansion.regShiftsMasks.otgEnable) != 0;
}

bool AMDRadeonX5000_AMDGFX9DCNDisplay::isDisplayInterlaceEnabled(AMDRadeonX5000_AMDHWDisplay* const _self,
                                                                 const UInt32                       fbIndex)
{
    const auto  self           = static_cast<AMDRadeonX5000_AMDGFX9DCNDisplay*>(_self);
    const auto& expansion      = self->getExpansion();
    const auto& regOffs        = expansion.regOffs[fbIndex];
    const auto& regShiftsMasks = expansion.regShiftsMasks;
    assert(regOffs.isValid);
    assert(regShiftsMasks.isValid);
    return (self->getHWRegisters()->read(regOffs.otgInterlaceControl) & regShiftsMasks.otgInterlaceEnable) != 0;
}

bool AMDRadeonX5000_AMDGFX9DCNDisplay::isFlipPending(AMDRadeonX5000_AMDHWDisplay* const _self, const UInt32 fbIndex)
{
    const auto  self      = static_cast<AMDRadeonX5000_AMDGFX9DCNDisplay*>(_self);
    const auto& expansion = self->getExpansion();
    const auto& regOffs   = expansion.regOffs[fbIndex];
    assert(regOffs.isValid);

    const UInt64 earliestInUse =
        self->getHWRegisters()->read(regOffs.hubpreqSurfaceEarliestInuse)
        | static_cast<UInt64>(self->getHWRegisters()->read(regOffs.hubpreqSurfaceEarliestInuseHigh)) << 32;

    return earliestInUse != expansion.lastSubmitFlipOffset;
}

// No, no, there's no DCN1 option.
AMDFlipOption AMDRadeonX5000_AMDGFX9DCNDisplay::getFlipOption(AMDRadeonX5000_AMDHWDisplay*)
{ return AMDFlipOption::DCN2; }

UInt32 AMDRadeonX5000_AMDGFX9DCNDisplay::getNumberOfSupportedDisplays(AMDRadeonX5000_AMDHWDisplay*)
{ return MAX_SUPPORTED_DISPLAYS_RV; }

void AMDRadeonX5000_AMDGFX9DCNDisplay::populateVFT(VFT& vft)
{
    vft.init(AMDRadeonX5000_AMDHWDisplay::vft());

    const auto vftInner                                       = static_cast<void*>(vft.inner());
    constants.vftInitializeRegisters(vftInner)                = initialiseRegisters;
    constants.vftRestoreRegisters(vftInner)                   = restoreRegisters;
    superGetDisplayInfo                                       = constants.vftGetDisplayInfo(vftInner);
    constants.vftGetDisplayInfo(vftInner)                     = getDisplayInfo;
    constants.vftWriteWaitForVLine(vftInner)                  = writeWaitForVLine;
    constants.vftInit(vftInner)                               = init;
    constants.vftGetPixelMode(vftInner)                       = getPixelMode;
    constants.vftGetPixelFormat(vftInner)                     = getPixelFormat;
    constants.vftWriteFlipParameters(vftInner)                = writeFlipParameters;
    constants.vftGetDisplayModeViewportSpecificInfo(vftInner) = getDisplayModeViewportSpecificInfo;
    constants.vftIsDisplayControlEnabled(vftInner)            = isDisplayControlEnabled;
    constants.vftIsDisplayInterlaceEnabled(vftInner)          = isDisplayInterlaceEnabled;
    if (currentKernelVersion() >= MACOS_11) { constants.vftGetFlipOption(vftInner) = getFlipOption; }

    if (currentKernelVersion() >= MACOS_13) { return; }
    constants.vftGetCurrentDisplayOffset(vftInner)   = getCurrentDisplayOffset;
    constants.vftSetCurrentDisplayOffset(vftInner)   = setCurrentDisplayOffset;
    constants.vftSetFlipControlRegister(vftInner)    = setFlipControlRegister;
    constants.vftWriteFlipControlRegisters(vftInner) = writeFlipControlRegisters;
    constants.vftIsFlipPending(vftInner)             = isFlipPending;

    if (currentKernelVersion() >= MACOS_10_15) { return; }
    constants.vftGetNumberOfSupportedDisplays(vftInner) = getNumberOfSupportedDisplays;
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::registerMC(const char* const kext, KernelPatcher& patcher, const size_t id,
                                                  const mach_vm_address_t slide, const size_t size)
{
    PenguinWizardry::RuntimeMCManager::singleton().registerMC(
        gRTMetaClass, kext, patcher, id, "__ZN27AMDRadeonX5000_AMDHWDisplay10gMetaClassE", slide, size);
}
