// AMD GFX 9 DCN Display implementation
// Derivative of AMDRadeonX5000 and AMDRadeonX6000 decompilation
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <GPUDriversAMD/Accel/HWDisplay.hpp>
#include <GPUDriversAMD/FB/Attributes.hpp>
#include <GPUDriversAMD/Packet3.hpp>
#include <PenguinWizardry/KernelVersion.hpp>
#include <iVega/AMDGFX9DCNDisplay.hpp>

PWDefineAbstractRuntimeMCWithExpansion(AMDRadeonX5000_AMDGFX9DCNDisplay, Expansion);

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
static void SET_HUBPREQ_FLIP_CONTROL_FLIP_TYPE(UInt32 &target, bool v) {
    target &= ~getBit<UInt32>(1);
    target |= static_cast<UInt32>(v) << 1;
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::initialiseRegisters(AMDRadeonX5000_AMDGFX9DCNDisplay *const self) {
    auto &expansion = self->getExpansion();
    for (size_t i = 0; i < self->supportedDisplayCount(); i++) {
        auto &savedState = expansion.savedState[i];
        const auto &regOffs = expansion.regOffs[i];
        assert(regOffs.isValid);
        savedState.hubpreqflipControl = self->getHWRegisters()->readReg32(regOffs.hubpreqFlipControl);
    }
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::restoreRegisters() {}

AMDRadeonX5000_AMDGFX9DCNDisplay::Constants AMDRadeonX5000_AMDGFX9DCNDisplay::constants {};

void AMDRadeonX5000_AMDGFX9DCNDisplay::setVrrTimestampInfoVentura(AMDRadeonX5000_AMDGFX9DCNDisplay &self,
    const UInt64 vTotalMin, const UInt64 vTotalMax, const UInt64 horizontalLineTime) {
    assert(currentKernelVersion() >= MACOS_13);

    self.vrrTimestampInfoVentura().lastTransactionTimestamp = 0;
    self.vrrTimestampInfoVentura().currentFrameStartTimestamp = 0;
    self.vrrTimestampInfoVentura().lastTransactionStartTime = 0;
    self.vrrTimestampInfoVentura().currentFrameVTotal = vTotalMin;
    self.vrrTimestampInfoVentura().horizontalLineTime = horizontalLineTime;
    self.vrrTimestampInfoVentura().currentFrameTime = 0;
    self.vrrTimestampInfoVentura().vTotalMin = vTotalMin;
    self.vrrTimestampInfoVentura().vTotalMax = vTotalMax;
    self.vrrTimestampInfoVentura().transactionOnGlassTime = 0;
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::setVrrTimestampInfo(AMDRadeonX5000_AMDGFX9DCNDisplay &self,
    const UInt64 vTotalMin, const UInt64 vTotalMax, const UInt64 horizontalLineTime) {
    assert(currentKernelVersion() < MACOS_13);

    self.vrrTimestampInfo().field0 = 0;
    self.vrrTimestampInfo().lastTransactionTimestamp = 0;
    self.vrrTimestampInfo().currentFrameStartTimestamp = 0;
    self.vrrTimestampInfo().lastTransactionStartTime = 0;
    self.vrrTimestampInfo().currentFrameVTotal = static_cast<UInt32>(vTotalMin);
    self.vrrTimestampInfo().horizontalLineTime = static_cast<UInt32>(horizontalLineTime);
    self.vrrTimestampInfo().currentFrameTime = 0;
    self.vrrTimestampInfo().vTotalMin = static_cast<UInt32>(vTotalMin);
    self.vrrTimestampInfo().vTotalMax = static_cast<UInt32>(vTotalMax);
    self.vrrTimestampInfo().transactionOnGlassTime = 0;
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::calcAndSetVrrTimestampInfo(AMDRadeonX5000_AMDGFX9DCNDisplay &self,
    FramebufferInfo *const fbInfo, IOTimingInformation &timingInfo) {
    assert(currentKernelVersion() >= MACOS_11);

    if (!fbInfo->isOnline) { return; }

    const auto vTotalMin = timingInfo.detailedInfo.v2.verticalBlanking + timingInfo.detailedInfo.v2.verticalActive;
    const auto vTotalMax = vTotalMin + timingInfo.detailedInfo.v2.verticalBlankingExtension;
    const auto pixelClock = timingInfo.detailedInfo.v2.pixelClock;
    if (pixelClock == 0) {
        constants.setVrrTimestampInfo(self, vTotalMin, vTotalMax, 0);
    } else {
        constants.setVrrTimestampInfo(self, vTotalMin, vTotalMax,
            (timingInfo.detailedInfo.v2.horizontalBlanking + timingInfo.detailedInfo.v2.horizontalActive) / pixelClock);
    }
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::calcAndSetVrrTimestampInfoDummy(AMDRadeonX5000_AMDGFX9DCNDisplay &,
    FramebufferInfo *const, IOTimingInformation &) {}

bool AMDRadeonX5000_AMDGFX9DCNDisplay::fixedSuperGetDisplayInfo(const UInt32 fbIndex, const bool isCRTEnabled,
    const bool ignoreCRTOffsetCheck, IOFramebuffer *const fb, FramebufferInfo *const fbInfo) {
    if (fb == nullptr || fbIndex >= this->supportedDisplayCount()) { return false; }

    fbInfo->crtOffset = 0;
    fbInfo->size = 0;
    fbInfo->crtSize = 0;
    fbInfo->pitch = 0;
    fbInfo->rect.width = 0;
    fbInfo->rect.height = 0;

    auto &displayState = this->displayStates()[fbIndex];

    displayState.framebuffer = fb;
    displayState.status.setIsEnabled(isCRTEnabled);
    displayState.status.setIsIOFBFlipEnabled(true);
    displayState.status.setIsAccelBacked(false);

    uintptr_t wsaa = -1ULL;
    if (fb->getAttribute(ATTRIBUTE_WSAA, &wsaa) == kIOReturnSuccess) {
        this->wsaaAttributes()[fbIndex] = static_cast<UInt32>(wsaa);
        displayState.status.setIsWSAASupported(true);
    } else {
        displayState.status.setIsWSAASupported(false);
    }

    uintptr_t dpt = 0;
    const auto dptRet = fb->getAttribute(ATTRIBUTE_DISPLAY_PIPE_TRANSACTION, &dpt);
    displayState.status.setIsDPTSupported(dptRet == kIOReturnSuccess || dptRet == kIOReturnNotReady);

    const auto crtOffset = OSDynamicCast(OSData, fb->getProperty("ATY,fb_offset"));
    if (crtOffset != nullptr) {
        const auto data = crtOffset->getBytesNoCopy();
        if (data != nullptr) { fbInfo->crtOffset = *static_cast<const UInt64 *>(data); }
    }

    const auto crtSize = OSDynamicCast(OSData, fb->getProperty("ATY,fb_size"));
    if (crtSize != nullptr) {
        const auto data = crtSize->getBytesNoCopy();
        if (data != nullptr) { fbInfo->crtSize = *static_cast<const UInt32 *>(data); }
    }

    auto aperture = fb->getApertureRange(kIOFBSystemAperture);
    auto vram = fb->getVRAMRange();

    fbInfo->isMapped = aperture != nullptr && vram != nullptr;

    [[clang::suppress]] OSSafeReleaseNULL(aperture);
    [[clang::suppress]] OSSafeReleaseNULL(vram);

    getDisplayModeViewportSpecificInfo(this, fbIndex, &this->viewportStartYs()[fbIndex],
        &this->viewportHeights()[fbIndex]);

    uintptr_t isOnline = 0;
    fbInfo->isOnline = fb->getAttributeForConnection(static_cast<IOIndex>(fbIndex), kConnectionEnable, &isOnline) ==
                           kIOReturnSuccess &&
                       isOnline != 0;

    this->fedsParamInfo()[fbIndex].crtIndex = 0;
    this->fedsParamInfo()[fbIndex].scaledW = 0;
    this->fedsParamInfo()[fbIndex].scaledH = 0;
    this->fedsParamInfo()[fbIndex].srcW = 0;
    this->fedsParamInfo()[fbIndex].srcH = 0;

    this->scalerFlags()[fbIndex] = 0;

    IODisplayModeID displayMode = 0;
    IOIndex depth = 0;
    if (fb->getCurrentDisplayMode(&displayMode, &depth) == kIOReturnSuccess) {
        if (fb->getPixelInformation(displayMode, depth, kIOFBSystemAperture, &displayState.pixelInfo) ==
            kIOReturnSuccess) {
            const auto bytesPerPixel = displayState.pixelInfo.bitsPerPixel / 8;
            if (bytesPerPixel != 0) { fbInfo->pitch = displayState.pixelInfo.bytesPerRow / bytesPerPixel; }
            fbInfo->rect.width = displayState.pixelInfo.activeWidth;
            fbInfo->rect.height = displayState.pixelInfo.activeHeight;
        }

        IODisplayModeInformation modeInfo {};
        if (fb->getInformationForDisplayMode(displayMode, &modeInfo) == kIOReturnSuccess &&
            (modeInfo.flags & kDisplayModeAcceleratorBackedFlag) != 0) {
            displayState.status.setIsAccelBacked(true);
            this->fedsParamInfo()[fbIndex].crtIndex = 1;
        }

        IOTimingInformation timingInfo {};
        timingInfo.flags = kIODetailedTimingValid;
        if (fb->getTimingInfoForDisplayMode(displayMode, &timingInfo) == kIOReturnSuccess &&
            (timingInfo.flags & kIODetailedTimingValid) != 0) {
            this->scalerFlags()[fbIndex] = timingInfo.detailedInfo.v2.scalerFlags;

            if (this->fedsParamInfo()[fbIndex].crtIndex == 1) {
                this->fedsParamInfo()[fbIndex].scaledW = timingInfo.detailedInfo.v2.horizontalActive;
                this->fedsParamInfo()[fbIndex].scaledH = timingInfo.detailedInfo.v2.verticalActive;
                this->fedsParamInfo()[fbIndex].srcW = timingInfo.detailedInfo.v2.horizontalScaled;
                this->fedsParamInfo()[fbIndex].srcH = timingInfo.detailedInfo.v2.verticalScaled;
            }
        }
    }

    auto &hwSpecificInfo = this->crtHWSpecificInfos()[fbIndex];

    auto ret = true;

    if (!isCRTEnabled ||
        (!ignoreCRTOffsetCheck && fbInfo->crtOffset >= this->getHWInterface()->getHWMemory()->getVisibleSize())) {
        fbInfo->crtOffset = 0;
        fbInfo->size = 0;
    } else {
        CRTHWDepth hwDepth;
        switch (displayState.pixelInfo.bitsPerPixel) {
            case 8: {
                hwDepth = CRTHWDepth::DEPTH_8;
            } break;
            case 16: {
                hwDepth = CRTHWDepth::DEPTH_16;
            } break;
            case 32: {
                hwDepth = CRTHWDepth::DEPTH_32;
            } break;
            case 64: {
                hwDepth = CRTHWDepth::DEPTH_64;
            } break;
            default: {
                hwDepth = CRTHWDepth::DEPTH_32;
                ret = false;
            } break;
        }
        DBGLOG("GFX9DCNDisplay", "%s hwDepth=%s for bpp %d", __func__, stringifyCRTHWDepth(hwDepth),
            displayState.pixelInfo.bitsPerPixel);
        hwSpecificInfo.graphDepth = hwDepth;

        CRTHWFormat hwFormat;    // bug fix - original code only handled depth 64 and format 10
        switch (displayState.pixelInfo.bitsPerComponent) {
            case 8: {
                hwFormat = CRTHWFormat::FORMAT_8;
            } break;
            case 10: {
                hwFormat = CRTHWFormat::FORMAT_10;
            } break;
            case 12: {
                hwFormat = CRTHWFormat::FORMAT_12;
            } break;
            default: {
                hwFormat = CRTHWFormat::FORMAT_8;
                ret = false;
            } break;
        }
        DBGLOG("GFX9DCNDisplay", "%s hwFormat=%s for bpc %d", __func__, stringifyCRTHWFormat(hwFormat),
            displayState.pixelInfo.bitsPerComponent);
        hwSpecificInfo.graphFormat = hwFormat;

        switch (hwDepth) {
            case CRTHWDepth::DEPTH_8: {
                hwSpecificInfo.bytesPerPixel = 1;
            } break;
            case CRTHWDepth::DEPTH_16: {
                hwSpecificInfo.bytesPerPixel = 2;
            } break;
            case CRTHWDepth::DEPTH_32: {
                hwSpecificInfo.bytesPerPixel = 4;
            } break;
            case CRTHWDepth::DEPTH_64: {
                hwSpecificInfo.bytesPerPixel = 8;
            } break;
        }
        hwSpecificInfo.pixelMode = getPixelMode(this, hwDepth, hwFormat);
        DBGLOG("GFX9DCNDisplay", "%s hwSpecificInfo.pixelMode=%s", __func__,
            stringifyATIPixelMode(hwSpecificInfo.pixelMode));
        hwSpecificInfo.format = getPixelFormat(this, hwSpecificInfo.pixelMode);
        DBGLOG("GFX9DCNDisplay", "%s hwSpecificInfo.format=%s", __func__, stringifyATIFormat(hwSpecificInfo.format));
        hwSpecificInfo.isInterlaced = isDisplayInterlaceEnabled(this, fbIndex);
        displayState.status.setIsInterlaced(hwSpecificInfo.isInterlaced);

        ADDR2_COMPUTE_SURFACE_INFO_INPUT surfInfoInput {};
        surfInfoInput.width = fbInfo->rect.width;
        surfInfoInput.height = fbInfo->rect.height;
        surfInfoInput.bpp = displayState.pixelInfo.bitsPerPixel;
        surfInfoInput.resourceType = ADDR_RSRC_TEX_2D;
        surfInfoInput.format = this->getHWInterface()->getHWAlignManager()->getAddrFormat(hwSpecificInfo.pixelMode);
        surfInfoInput.numSamples = 1;
        surfInfoInput.numSlices = 1;
        surfInfoInput.flags.display = true;
        surfInfoInput.swizzleMode = surfInfoInput.format;                  // TODO:
        this->savedSwizzleModes()[fbIndex] = surfInfoInput.swizzleMode;    // Use `Addr2GetPreferredSurfaceSetting`.
        this->swizzleModes()[fbIndex] = surfInfoInput.swizzleMode;
        this->savedResourceTypes()[fbIndex] = surfInfoInput.resourceType;
        if (this->getHWInterface()->getHWAlignManager()->getSurfaceInfo2(&surfInfoInput,
                &this->surfInfoOutputs()[fbIndex]) == kIOReturnSuccess) {
            fbInfo->size = fbInfo->pitch * this->surfInfoOutputs()[fbIndex].height * hwSpecificInfo.bytesPerPixel;
        } else {
            fbInfo->size = 0;
        }
        displayState.status.setIsEnabled(true);
    }

    AMDHWDisplayState::Status combinedStatus {};
    for (UInt32 i = 0; i < this->supportedDisplayCount(); i += 1) { combinedStatus |= this->displayStates()[i].status; }
    this->combinedStatus() = combinedStatus;

    fbInfo->savedSize = fbInfo->size;

    if (this->fedsParamInfo()[fbIndex].crtIndex != 0) {
        ADDR2_COMPUTE_SURFACE_INFO_INPUT surfInfoInput {};
        surfInfoInput.width = this->fedsParamInfo()[fbIndex].scaledW;
        surfInfoInput.height = this->fedsParamInfo()[fbIndex].scaledH;
        surfInfoInput.bpp = displayState.pixelInfo.bitsPerPixel;
        surfInfoInput.swizzleMode = this->savedSwizzleModes()[fbIndex];
        surfInfoInput.resourceType = this->savedResourceTypes()[fbIndex];
        surfInfoInput.format = this->getHWInterface()->getHWAlignManager()->getAddrFormat(hwSpecificInfo.pixelMode);
        surfInfoInput.numSamples = 1;
        surfInfoInput.numSlices = 1;
        surfInfoInput.flags.display = true;
        ADDR2_COMPUTE_SURFACE_INFO_OUTPUT surfInfoOutput {};
        if (this->getHWInterface()->getHWAlignManager()->getSurfaceInfo2(&surfInfoInput, &surfInfoOutput) ==
            kIOReturnSuccess) {
            fbInfo->savedSize = surfInfoOutput.height * surfInfoOutput.pitch * hwSpecificInfo.bytesPerPixel;
        }
    }

    UInt64 baseAlign = this->surfInfoOutputs()[fbIndex].baseAlign;
    UInt8 shift = 0;
    while (page_size < baseAlign) {
        baseAlign >>= 1;
        shift += 1;
    }
    fbInfo->pageCount = alignValue(page_size << shift);

    return ret;
}

bool AMDRadeonX5000_AMDGFX9DCNDisplay::getDisplayInfo(AMDRadeonX5000_AMDGFX9DCNDisplay *const self,
    const UInt32 fbIndex, const bool isCRTEnabled, const bool ignoreCRTOffsetCheck, IOFramebuffer *const fb,
    FramebufferInfo *const fbInfo) {
    if (!self->fixedSuperGetDisplayInfo(fbIndex, isCRTEnabled, ignoreCRTOffsetCheck, fb, fbInfo)) { return false; }

    if (!isCRTEnabled) { return true; }

    auto &expansion = self->getExpansion();
    auto &savedState = expansion.savedState[fbIndex];
    const auto &displayState = self->displayStates()[fbIndex];

    bzero(&savedState.flipParam, sizeof(savedState.flipParam));

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
            default:
                return false;
        }
    }

    UInt32 hwPixelFormat;
    AMDHWSurfacePixelFormat pixelFormat;
    switch (displayState.pixelInfo.bitsPerPixel) {
        case 16: {
            hwPixelFormat = 1;
            pixelFormat = AMDHWSurfacePixelFormat::ARGB1555;
        } break;
        case 32: {
            if (displayState.pixelInfo.bitsPerComponent == 10) {
                hwPixelFormat = 10;
                pixelFormat = AMDHWSurfacePixelFormat::ARGB2101010;
            } else {
                hwPixelFormat = 8;
                pixelFormat = AMDHWSurfacePixelFormat::ARGB8888;
            }
        } break;
        case 64: {
            hwPixelFormat = 24;
            pixelFormat = AMDHWSurfacePixelFormat::ARGB16161616F;
        } break;
        default:
            return false;
    }

    const auto addrConfig = self->getHWInterface()->getAddrConfig();
    savedState.hubpSurfaceConfig =
        SURFACE_CONFIG_PIXEL_FORMAT(hwPixelFormat) | SURFACE_CONFIG_ROTATION_ANGLE(static_cast<UInt32>(hwRotation));
    savedState.hubpControl = 0;
    savedState.hubpAddrConfig =
        HUBP_ADDR_CONFIG_NUM_PIPES(GET_GB_ADDR_CONFIG_NUM_PIPES(addrConfig)) |
        HUBP_ADDR_CONFIG_NUM_BANKS(GET_GB_ADDR_CONFIG_NUM_BANKS(addrConfig)) |
        HUBP_ADDR_CONFIG_PIPE_INTERLEAVE(GET_GB_ADDR_CONFIG_PIPE_INTERLEAVE(addrConfig)) |
        HUBP_ADDR_CONFIG_NUM_SE(GET_GB_ADDR_CONFIG_NUM_SE(addrConfig)) |
        HUBP_ADDR_CONFIG_NUM_RB_PER_SE(GET_GB_ADDR_CONFIG_NUM_RB_PER_SE(addrConfig)) |
        HUBP_ADDR_CONFIG_MAX_COMPRESSED_FRAGS(GET_GB_ADDR_CONFIG_MAX_COMPRESSED_FRAGS(addrConfig));
    savedState.hubpTilingConfig = TILING_CONFIG_SW_MODE(self->swizzleModes()[fbIndex]) | TILING_CONFIG_DIM_TYPE(1);
    savedState.hwRotation = hwRotation;
    savedState.flipParam.dcn.surfaceFormat = pixelFormat;
    savedState.flipParam.dcn.surfaceRotation = hwRotation;
    self->fillFlipTilingParameters(&savedState.flipParam, self->swizzleModes()[fbIndex]);

    return true;
}

UInt64 AMDRadeonX5000_AMDGFX9DCNDisplay::getCurrentDisplayOffset(AMDRadeonX5000_AMDGFX9DCNDisplay *const self,
    const UInt32 fbIndex) {
    auto &expansion = self->getExpansion();
    const auto &regOffs = expansion.regOffs[fbIndex];
    assert(regOffs.isValid);

    return static_cast<UInt64>(self->getHWRegisters()->readReg32(regOffs.hubpreqPrimarySurfaceAddressHigh)) << 32 |
           self->getHWRegisters()->readReg32(regOffs.hubpreqPrimarySurfaceAddress);
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::setCurrentDisplayOffset(AMDRadeonX5000_AMDGFX9DCNDisplay *const self,
    const UInt32 fbIndex, const UInt64 value) {
    auto &expansion = self->getExpansion();
    const auto &regOffs = expansion.regOffs[fbIndex];
    assert(regOffs.isValid);
    assert(expansion.regShiftsMasks.isValid);
    self->setFlipControlRegister(self, fbIndex, AMDSwapInterval::Immediate);
    self->getHWRegisters()->writeReg32(regOffs.hubpreqPrimarySurfaceAddressHigh,
        (value >> 32) & expansion.regShiftsMasks.primarySurfaceHi);
    self->getHWRegisters()->writeReg32(regOffs.hubpreqPrimarySurfaceAddress, value & 0xFFFFFFFF);
    expansion.lastSubmitFlipOffset = value;
    while (self->isFlipPending(self, fbIndex)) { IODelay(100); }
}

UInt32 AMDRadeonX5000_AMDGFX9DCNDisplay::writeWaitForVLine(AMDRadeonX5000_AMDGFX9DCNDisplay *, UInt32 *const,
    const UInt32, SInt32 &, SInt32 &, const bool, const bool) {
    assert(false);
    return 0;
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::setFlipControlRegister(AMDRadeonX5000_AMDGFX9DCNDisplay *const self,
    const UInt32 fbIndex, const AMDSwapInterval swapInterval) {
    auto &expansion = self->getExpansion();
    const auto &regOffs = expansion.regOffs[fbIndex];
    assert(regOffs.isValid);
    auto &savedState = expansion.savedState[fbIndex];
    SET_HUBPREQ_FLIP_CONTROL_FLIP_TYPE(savedState.hubpreqflipControl, swapInterval == AMDSwapInterval::Immediate);
    self->getHWRegisters()->writeReg32(regOffs.hubpreqFlipControl, savedState.hubpreqflipControl);
}

bool AMDRadeonX5000_AMDGFX9DCNDisplay::init(AMDRadeonX5000_AMDGFX9DCNDisplay *const self, void *const hwInterface,
    void *const fbParams) {
    assert(AMDRadeonX5000_AMDHWDisplay::init() != 0);
    if (!FunctionCast(init, AMDRadeonX5000_AMDHWDisplay::init())(self, hwInterface, fbParams)) { return false; }

    self->isDCN() = true;

    self->initDCNRegOffs();

    return true;
}

// TODO: maybe handle more of these?
ATIPixelMode AMDRadeonX5000_AMDGFX9DCNDisplay::getPixelMode(AMDRadeonX5000_AMDGFX9DCNDisplay *const,
    const CRTHWDepth depth, const CRTHWFormat format) {
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

ATIFormat AMDRadeonX5000_AMDGFX9DCNDisplay::getPixelFormat(AMDRadeonX5000_AMDGFX9DCNDisplay *const,
    const ATIPixelMode pixelMode) {
    DBGLOG("GFX9DCNDisplay", "%s << (pixelMode: %s)", __func__, stringifyATIPixelMode(pixelMode));
    switch (pixelMode) {
        case ATIPixelMode::C_8:
            return ATIFormat::BGRA8;
        case ATIPixelMode::C_2_10_10_10:
            return ATIFormat::ARGB2101010;
        case ATIPixelMode::C_16_16_16_16:
            // return ATIFormat::ARGB16161616F; // GFX10
            return ATIFormat::BGRA16;    // GFX9
        case ATIPixelMode::C_5_6_5:
        case ATIPixelMode::C_1_5_5_5:
            return ATIFormat::ARGB1555;
        default:
            return ATIFormat::Luminance8;
    }
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::fillFlipTilingParameters(AMDFlipParam *const flipParam,
    const UInt32 swizzleMode) {
    auto addrConfig = this->getHWInterface()->getAddrConfig();
    flipParam->dcn.tilingParams.dcn2.numPipes = GET_GB_ADDR_CONFIG_NUM_PIPES(addrConfig);
    flipParam->dcn.tilingParams.dcn2.numBanks = GET_GB_ADDR_CONFIG_NUM_BANKS(addrConfig);
    flipParam->dcn.tilingParams.dcn2.pipeInter = GET_GB_ADDR_CONFIG_PIPE_INTERLEAVE(addrConfig);
    flipParam->dcn.tilingParams.dcn2.numShaderEngines = GET_GB_ADDR_CONFIG_NUM_SE(addrConfig);
    flipParam->dcn.tilingParams.dcn2.numRbPerSe = GET_GB_ADDR_CONFIG_NUM_RB_PER_SE(addrConfig);
    flipParam->dcn.tilingParams.dcn2.maxFrags = getBit(GET_GB_ADDR_CONFIG_MAX_COMPRESSED_FRAGS(addrConfig));
    flipParam->dcn.tilingParams.dcn2.swizzleMode = swizzleMode;
}

bool AMDRadeonX5000_AMDGFX9DCNDisplay::writeFlipParameters(AMDRadeonX5000_AMDGFX9DCNDisplay *const self,
    AMDPipeFlip *const flip, const UInt32 fbIndex, const UInt64 offsetLeft, const UInt64 offsetRight,
    const AMDSwapInterval swapInterval, const UInt32 pitch, const AMDTilingInfo *const tileInfo,
    const ATIFormat atiFormat, const UInt32, AMDHWRotationAngle *const hwRotation, AMDPipeFlip *const savedFlip) {
    assert(offsetRight == 0);

    auto &expansion = self->getExpansion();
    auto &savedState = expansion.savedState[fbIndex];

    AMDFlipParam newFlipParam;
    bzero(&newFlipParam, sizeof(newFlipParam));

    switch (atiFormat) {
        case ATIFormat::RGB565: {
            newFlipParam.dcn.surfaceFormat = AMDHWSurfacePixelFormat::RGB565;
        } break;
        case ATIFormat::BGR565: {
            newFlipParam.dcn.surfaceFormat = AMDHWSurfacePixelFormat::BGR565;
        } break;
        case ATIFormat::ARGB1555: {
            newFlipParam.dcn.surfaceFormat = AMDHWSurfacePixelFormat::ABGR1555;
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
        default:
            return false;
    }

    if (hwRotation == nullptr) {
        newFlipParam.dcn.surfaceRotation = savedState.flipParam.dcn.surfaceRotation;
    } else {
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
            default:
                return false;
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
        newFlipParam.dcn.surfaceFormat = savedState.flipParam.dcn.surfaceFormat;
        newFlipParam.dcn.surfaceRotation = savedState.flipParam.dcn.surfaceRotation;
        newFlipParam.dcn.tilingParams.dcn2 = savedState.flipParam.dcn.tilingParams.dcn2;
    } else {
        self->fillFlipTilingParameters(&newFlipParam, tileInfo->tilingConfig & 0x1F);
    }

    newFlipParam.dcn.surfacePitch = pitch;
    newFlipParam.dcn.surfaceAddr = offsetLeft;

    if (savedFlip != nullptr && swapInterval != AMDSwapInterval::VSync) {
        if (savedFlip->flipParam.dcn.surfaceFormat != newFlipParam.dcn.surfaceFormat ||
            savedFlip->flipParam.dcn.surfaceRotation != newFlipParam.dcn.surfaceRotation ||
            savedState.flipParam.dcn.tilingParams.dcn2 != newFlipParam.dcn.tilingParams.dcn2) {
            return false;
        }
    }

    if (flip != nullptr) { flip->flipParam = newFlipParam; }
    if (savedFlip != nullptr) { savedFlip->flipParam = newFlipParam; }

    return true;
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::getDisplayModeViewportSpecificInfo(AMDRadeonX5000_AMDGFX9DCNDisplay *const self,
    const UInt32 fbIndex, UInt32 *const viewportYStart, UInt32 *const viewportHeight) {
    auto &expansion = self->getExpansion();
    const auto &regOffs = expansion.regOffs[fbIndex];
    const auto &regShiftsMasks = expansion.regShiftsMasks;
    assert(regOffs.isValid);
    assert(regShiftsMasks.isValid);
    if (viewportYStart != nullptr) {
        *viewportYStart =
            (self->getHWRegisters()->readReg32(regOffs.hubpPriViewportStart) & regShiftsMasks.viewportYStartMask) >>
            regShiftsMasks.viewportYStartShift;
    }
    if (viewportHeight != nullptr) {
        *viewportHeight =
            (self->getHWRegisters()->readReg32(regOffs.hubpPriViewportDimension) & regShiftsMasks.viewportHeightMask) >>
            regShiftsMasks.viewportHeightShift;
    }
}

UInt32 AMDRadeonX5000_AMDGFX9DCNDisplay::writeFlipControlRegisters(AMDRadeonX5000_AMDGFX9DCNDisplay *const self,
    const UInt32 fbIndex, UInt32 *const buffer, const AMDSwapInterval swapInterval, const UInt64 offsetLeft,
    const UInt64 offsetRight) {
    assert(offsetRight == 0);

    UInt32 displays[MAX_SUPPORTED_DISPLAYS];
    displays[0] = fbIndex;
    const auto displayCount = self->getMirroredDisplays(fbIndex, &displays[1]) + 1;
    const auto &expansion = self->getExpansion();
    const auto flipControl = HUBPREQ_FLIP_CONTROL_FLIP_TYPE(swapInterval == AMDSwapInterval::Immediate);
    UInt32 dwordCount = 0;
    const auto &regShiftsMasks = expansion.regShiftsMasks;
    assert(regShiftsMasks.isValid);
    for (UInt32 i = 0; i < displayCount; i++) {
        const auto &regOffs = expansion.regOffs[i];
        assert(regOffs.isValid);
        const auto &savedState = expansion.savedState[i];
        dwordCount += write1RegWritePacket(&buffer[dwordCount], regOffs.hubpreqFlipControl, flipControl);
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

bool AMDRadeonX5000_AMDGFX9DCNDisplay::isDisplayControlEnabled(AMDRadeonX5000_AMDGFX9DCNDisplay *const self,
    const UInt32 fbIndex) {
    const auto &expansion = self->getExpansion();
    const auto &regOffs = expansion.regOffs[fbIndex];
    assert(regOffs.isValid);
    assert(expansion.regShiftsMasks.isValid);
    return (self->getHWRegisters()->readReg32(regOffs.otgControl) & expansion.regShiftsMasks.otgEnable) != 0;
}

bool AMDRadeonX5000_AMDGFX9DCNDisplay::isDisplayInterlaceEnabled(AMDRadeonX5000_AMDGFX9DCNDisplay *const self,
    const UInt32 fbIndex) {
    const auto &expansion = self->getExpansion();
    const auto &regOffs = expansion.regOffs[fbIndex];
    const auto &regShiftsMasks = expansion.regShiftsMasks;
    assert(regOffs.isValid);
    assert(regShiftsMasks.isValid);
    return (self->getHWRegisters()->readReg32(regOffs.otgInterlaceControl) & regShiftsMasks.otgInterlaceEnable) != 0;
}

bool AMDRadeonX5000_AMDGFX9DCNDisplay::isFlipPending(AMDRadeonX5000_AMDGFX9DCNDisplay *const self,
    const UInt32 fbIndex) {
    const auto &expansion = self->getExpansion();
    const auto &regOffs = expansion.regOffs[fbIndex];
    assert(regOffs.isValid);

    const UInt64 earliestInUse =
        self->getHWRegisters()->readReg32(regOffs.hubpreqSurfaceEarliestInuse) |
        static_cast<UInt64>(self->getHWRegisters()->readReg32(regOffs.hubpreqSurfaceEarliestInuseHigh)) << 32;

    return earliestInUse != expansion.lastSubmitFlipOffset;
}

// No, no, there's no DCN1 option.
AMDFlipOption AMDRadeonX5000_AMDGFX9DCNDisplay::getFlipOption() { return AMDFlipOption::DCN2; }

void AMDRadeonX5000_AMDGFX9DCNDisplay::populateVFT(RuntimeVFT<vftCount, 1> &vft) {
    assert(AMDRadeonX5000_AMDHWDisplay::vfuncs() != nullptr);
    vft.init(AMDRadeonX5000_AMDHWDisplay::vfuncs());

    if (currentKernelVersion() >= MACOS_13) {
        vft.get<decltype(initialiseRegisters)>(INITIALIZE_REGISTERS_VT_INDEX) = initialiseRegisters;
        vft.get<decltype(restoreRegisters)>(RESTORE_REGISTERS_VT_INDEX) = restoreRegisters;
        vft.get<decltype(getDisplayInfo)>(GET_DISPLAY_INFO_VT_INDEX) = getDisplayInfo;
        vft.get<decltype(writeWaitForVLine)>(WRITE_WAIT_FOR_VLINE_VT_INDEX_MAC13) = writeWaitForVLine;
        vft.get<decltype(init)>(INIT_VT_INDEX_MAC13) = init;
        vft.get<decltype(getPixelMode)>(GET_PIXEL_MODE_VT_INDEX_MAC13) = getPixelMode;
        vft.get<decltype(getPixelFormat)>(GET_PIXEL_FORMAT_VT_INDEX_MAC13) = getPixelFormat;
        vft.get<decltype(writeFlipParameters)>(WRITE_FLIP_PARAMETERS_VT_INDEX_MAC13) = writeFlipParameters;
        vft.get<decltype(getDisplayModeViewportSpecificInfo)>(GET_DISPLAY_MODE_VIEWPORT_SPECIFIC_INFO_VT_INDEX_MAC13) =
            getDisplayModeViewportSpecificInfo;
        vft.get<decltype(isDisplayControlEnabled)>(IS_DISPLAY_CONTROL_ENABLED_VT_INDEX_MAC13) = isDisplayControlEnabled;
        vft.get<decltype(isDisplayInterlaceEnabled)>(IS_DISPLAY_INTERLACE_ENABLED_VT_INDEX_MAC13) =
            isDisplayInterlaceEnabled;
        vft.get<decltype(getFlipOption)>(GET_FLIP_OPTION_VT_INDEX_MAC13) = getFlipOption;
        return;
    }

    vft.get<decltype(initialiseRegisters)>(INITIALIZE_REGISTERS_VT_INDEX) = initialiseRegisters;
    vft.get<decltype(restoreRegisters)>(RESTORE_REGISTERS_VT_INDEX) = restoreRegisters;
    vft.get<decltype(getDisplayInfo)>(GET_DISPLAY_INFO_VT_INDEX) = getDisplayInfo;
    vft.get<decltype(getCurrentDisplayOffset)>(GET_CURRENT_DISPLAY_OFFSET_VT_INDEX) = getCurrentDisplayOffset;
    vft.get<decltype(setCurrentDisplayOffset)>(SET_CURRENT_DISPLAY_OFFSET_VT_INDEX) = setCurrentDisplayOffset;
    vft.get<decltype(writeWaitForVLine)>(WRITE_WAIT_FOR_VLINE_VT_INDEX) = writeWaitForVLine;
    vft.get<decltype(setFlipControlRegister)>(SET_FLIP_CONTROL_REGISTER_VT_INDEX) = setFlipControlRegister;
    vft.get<decltype(getPixelMode)>(GET_PIXEL_MODE_VT_INDEX) = getPixelMode;
    vft.get<decltype(getPixelFormat)>(GET_PIXEL_FORMAT_VT_INDEX) = getPixelFormat;
    vft.get<decltype(writeFlipParameters)>(WRITE_FLIP_PARAMETERS_VT_INDEX) = writeFlipParameters;
    vft.get<decltype(getDisplayModeViewportSpecificInfo)>(GET_DISPLAY_MODE_VIEWPORT_SPECIFIC_INFO_VT_INDEX) =
        getDisplayModeViewportSpecificInfo;
    vft.get<decltype(writeFlipControlRegisters)>(WRITE_FLIP_CONTROL_REGISTERS_VT_INDEX) = writeFlipControlRegisters;
    vft.get<decltype(isDisplayControlEnabled)>(IS_DISPLAY_CONTROL_ENABLED_VT_INDEX) = isDisplayControlEnabled;
    vft.get<decltype(isDisplayInterlaceEnabled)>(IS_DISPLAY_INTERLACE_ENABLED_VT_INDEX) = isDisplayInterlaceEnabled;
    vft.get<decltype(isFlipPending)>(IS_FLIP_PENDING_VT_INDEX) = isFlipPending;
    if (currentKernelVersion() == MACOS_10_15) {
        vft.get<decltype(init)>(INIT_VT_INDEX_MAC10_15) = init;
        return;
    }
    vft.get<decltype(init)>(INIT_VT_INDEX) = init;
    vft.get<decltype(getFlipOption)>(GET_FLIP_OPTION_VT_INDEX) = getFlipOption;
}

void AMDRadeonX5000_AMDGFX9DCNDisplay::registerMC(const char *const kext, KernelPatcher &patcher, const size_t id,
    const mach_vm_address_t slide, const size_t size) {
    PenguinWizardry::RuntimeMCManager::singleton().registerMC(gRTMetaClass, kext, patcher, id,
        "__ZN27AMDRadeonX5000_AMDHWDisplay10gMetaClassE", slide, size);
}
