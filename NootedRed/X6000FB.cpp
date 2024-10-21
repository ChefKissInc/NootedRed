// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include "X6000FB.hpp"
#include "AMDCommon.hpp"
#include "ATOMBIOS.hpp"
#include "NRed.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>

constexpr UInt32 FbAttributeBacklight = static_cast<UInt32>('bklt');

static const char *pathRadeonX6000Framebuffer =
    "/System/Library/Extensions/AMDRadeonX6000Framebuffer.kext/Contents/MacOS/AMDRadeonX6000Framebuffer";

static KernelPatcher::KextInfo kextRadeonX6000Framebuffer {
    "com.apple.kext.AMDRadeonX6000Framebuffer",
    &pathRadeonX6000Framebuffer,
    1,
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

X6000FB *X6000FB::callback = nullptr;

void X6000FB::init() {
    switch (getKernelVersion()) {
        case KernelVersion::Catalina:
            this->dcLinkCapsField = 0x1EA;
            break;
        case KernelVersion::BigSur:
            this->dcLinkCapsField = 0x26C;
            break;
        case KernelVersion::Monterey:
            this->dcLinkCapsField = 0x284;
            break;
        case KernelVersion::Ventura:
        case KernelVersion::Sonoma:
        case KernelVersion::Sequoia:
            this->dcLinkCapsField = 0x28C;
            break;
        default:
            PANIC("X6000FB", "Unsupported kernel version %d", getKernelVersion());
    }

    SYSLOG("X6000FB", "Module initialised");

    callback = this;
    lilu.onKextLoadForce(&kextRadeonX6000Framebuffer);
}

bool X6000FB::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX6000Framebuffer.loadIndex == id) {
        NRed::callback->hwLateInit();

        CAILAsicCapsEntry *orgAsicCapsTable;
        SolveRequestPlus solveRequest {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable, kCailAsicCapsTablePattern};
        PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "X6000FB", "Failed to resolve CAIL_ASIC_CAPS_TABLE");

        if (NRed::callback->attributes.isBacklightEnabled()) {
            if (NRed::callback->attributes.isBigSurAndLater() && NRed::callback->attributes.isRaven()) {
                SolveRequestPlus solveRequest {"_dce_driver_set_backlight", this->orgDceDriverSetBacklight,
                    kDceDriverSetBacklightPattern, kDceDriverSetBacklightPatternMask};
                PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "X6000FB",
                    "Failed to resolve dce_driver_set_backlight");
            }
            if (NRed::callback->attributes.isSonoma1404AndLater()) {
                SolveRequestPlus solveRequest {"_dc_link_set_backlight_level", this->orgDcLinkSetBacklightLevel,
                    kDcLinkSetBacklightLevelPattern14_4};
                PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "X6000FB",
                    "Failed to resolve dc_link_set_backlight_level");
            } else {
                SolveRequestPlus solveRequest {"_dc_link_set_backlight_level", this->orgDcLinkSetBacklightLevel,
                    kDcLinkSetBacklightLevelPattern};
                PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "X6000FB",
                    "Failed to resolve dc_link_set_backlight_level");
            }

            SolveRequestPlus solveRequest {"_dc_link_set_backlight_level_nits", this->orgDcLinkSetBacklightLevelNits,
                kDcLinkSetBacklightLevelNitsPattern, kDcLinkSetBacklightLevelNitsMask};
            PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "X6000FB",
                "Failed to resolve dc_link_set_backlight_level_nits");
        }

        if (NRed::callback->attributes.isVenturaAndLater()) {
            SolveRequestPlus solveRequest {
                "__ZNK34AMDRadeonX6000_AmdRadeonController18messageAcceleratorE25_eAMDAccelIOFBRequestTypePvS1_S1_",
                this->orgMessageAccelerator};
            PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "X6000FB",
                "Failed to resolve messageAccelerator");
        }

        RouteRequestPlus requests[] = {
            {"__ZNK15AmdAtomVramInfo16populateVramInfoER16AtomFirmwareInfo", wrapPopulateVramInfo,
                kPopulateVramInfoPattern, kPopulateVramInfoMask},
            {"__ZNK32AMDRadeonX6000_AmdAsicInfoNavi1027getEnumeratedRevisionNumberEv", wrapGetEnumeratedRevision},
            {"__ZNK22AmdAtomObjectInfo_V1_421getNumberOfConnectorsEv", wrapGetNumberOfConnectors,
                this->orgGetNumberOfConnectors, kGetNumberOfConnectorsPattern, kGetNumberOfConnectorsMask},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "X6000FB",
            "Failed to route symbols");

        if (checkKernelArgument("-NRedDPDelay")) {
            if (NRed::callback->attributes.isSonoma1404AndLater()) {
                RouteRequestPlus request {"_dp_receiver_power_ctrl", wrapDpReceiverPowerCtrl,
                    this->orgDpReceiverPowerCtrl, kDpReceiverPowerCtrl14_4};
                PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB",
                    "Failed to route dp_receiver_power_ctrl (14.4+)");
            } else {
                RouteRequestPlus request {"_dp_receiver_power_ctrl", wrapDpReceiverPowerCtrl,
                    this->orgDpReceiverPowerCtrl, kDpReceiverPowerCtrl};
                PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB",
                    "Failed to route dp_receiver_power_ctrl");
            }
        }

        if (ADDPR(debugEnabled)) {
            RouteRequestPlus requests[] = {
                {"__ZN24AMDRadeonX6000_AmdLogger15initWithPciInfoEP11IOPCIDevice", wrapInitWithPciInfo,
                    this->orgInitWithPciInfo},
                {"__ZN34AMDRadeonX6000_AmdRadeonController10doGPUPanicEPKcz", wrapDoGPUPanic},
                {"_dm_logger_write", wrapDmLoggerWrite, kDmLoggerWritePattern},
            };
            PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "X6000FB",
                "Failed to route debug symbols");
        }

        if (NRed::callback->attributes.isRenoir()) {
            RouteRequestPlus request {"_IH_4_0_IVRing_InitHardware", wrapIH40IVRingInitHardware,
                this->orgIH40IVRingInitHardware, kIH40IVRingInitHardwarePattern, kIH40IVRingInitHardwareMask};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB",
                "Failed to route IH_4_0_IVRing_InitHardware");
            if (NRed::callback->attributes.isSonoma1404AndLater()) {
                RouteRequestPlus request {"_IRQMGR_WriteRegister", wrapIRQMGRWriteRegister,
                    this->orgIRQMGRWriteRegister, kIRQMGRWriteRegisterPattern14_4};
                PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB",
                    "Failed to route IRQMGR_WriteRegister (14.4+)");
            } else {
                RouteRequestPlus request {"_IRQMGR_WriteRegister", wrapIRQMGRWriteRegister,
                    this->orgIRQMGRWriteRegister, kIRQMGRWriteRegisterPattern};
                PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB", "Failed to route IRQMGR_WriteRegister");
            }
        }

        if (NRed::callback->attributes.isVenturaAndLater()) {
            RouteRequestPlus request {"__ZN34AMDRadeonX6000_AmdRadeonController7powerUpEv", wrapControllerPowerUp,
                this->orgControllerPowerUp};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB", "Failed to route powerUp");
        }

        if (NRed::callback->attributes.isBacklightEnabled()) {
            if (NRed::callback->attributes.isBigSurAndLater() && NRed::callback->attributes.isRaven()) {
                if (NRed::callback->attributes.isSonoma1404AndLater()) {
                    RouteRequestPlus request = {"_dce_panel_cntl_hw_init", wrapDcePanelCntlHwInit,
                        this->orgDcePanelCntlHwInit, kDcePanelCntlHwInitPattern14_4};
                    PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB",
                        "Failed to route dce_panel_cntl_hw_init (14.4+)");
                } else {
                    RouteRequestPlus request = {"_dce_panel_cntl_hw_init", wrapDcePanelCntlHwInit,
                        this->orgDcePanelCntlHwInit, kDcePanelCntlHwInitPattern};
                    PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB",
                        "Failed to route dce_panel_cntl_hw_init");
                }
            }
            RouteRequestPlus requests[] = {
                {"_link_create", wrapLinkCreate, this->orgLinkCreate, kLinkCreatePattern, kLinkCreateMask},
                {"__ZN35AMDRadeonX6000_AmdRadeonFramebuffer25setAttributeForConnectionEijm",
                    wrapSetAttributeForConnection, this->orgSetAttributeForConnection},
                {"__ZN35AMDRadeonX6000_AmdRadeonFramebuffer25getAttributeForConnectionEijPm",
                    wrapGetAttributeForConnection, this->orgGetAttributeForConnection},
            };
            PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "X6000FB",
                "Failed to route backlight symbols");
        }

        const LookupPatchPlus patch {&kextRadeonX6000Framebuffer, kPopulateDeviceInfoOriginal, kPopulateDeviceInfoMask,
            kPopulateDeviceInfoPatched, kPopulateDeviceInfoMask, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB", "Failed to apply populateDeviceInfo patch");

        if (NRed::callback->attributes.isSonoma1404AndLater()) {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX6000Framebuffer, kGetFirmwareInfoNullCheckOriginal14_4,
                    kGetFirmwareInfoNullCheckOriginalMask14_4, kGetFirmwareInfoNullCheckPatched14_4,
                    kGetFirmwareInfoNullCheckPatchedMask14_4, 1},
                {&kextRadeonX6000Framebuffer, kGetVendorInfoOriginal14_4, kGetVendorInfoMask14_4,
                    kGetVendorInfoPatched14_4, kGetVendorInfoPatchedMask14_4, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "X6000FB",
                "Failed to apply patches (14.4)");
        } else {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX6000Framebuffer, kGetFirmwareInfoNullCheckOriginal, kGetFirmwareInfoNullCheckOriginalMask,
                    kGetFirmwareInfoNullCheckPatched, kGetFirmwareInfoNullCheckPatchedMask, 1},
                {&kextRadeonX6000Framebuffer, kGetVendorInfoOriginal, kGetVendorInfoMask, kGetVendorInfoPatched,
                    kGetVendorInfoPatchedMask, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "X6000FB", "Failed to apply patches");
        }

        if (NRed::callback->attributes.isCatalina()) {
            const LookupPatchPlus patch {&kextRadeonX6000Framebuffer, kAmdAtomVramInfoNullCheckCatalinaOriginal,
                kAmdAtomVramInfoNullCheckCatalinaMask, kAmdAtomVramInfoNullCheckCatalinaPatched, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB", "Failed to apply null check patch");
        } else {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX6000Framebuffer, kAmdAtomVramInfoNullCheckOriginal, kAmdAtomVramInfoNullCheckPatched, 1},
                {&kextRadeonX6000Framebuffer, kAmdAtomPspDirectoryNullCheckOriginal,
                    kAmdAtomPspDirectoryNullCheckPatched, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "X6000FB",
                "Failed to apply null check patches");
        }

        if (NRed::callback->attributes.isVenturaAndLater()) {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX6000Framebuffer, kControllerPowerUpOriginal, kControllerPowerUpOriginalMask,
                    kControllerPowerUpReplace, kControllerPowerUpReplaceMask, 1},
                {&kextRadeonX6000Framebuffer, kValidateDetailedTimingOriginal, kValidateDetailedTimingPatched, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "X6000FB",
                "Failed to apply logic revert patches");
        }

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X6000FB",
            "Failed to enable kernel writing");
        *orgAsicCapsTable = {
            .familyId = AMDGPU_FAMILY_RAVEN,
            .caps = NRed::callback->attributes.isRenoir() ? ddiCapsRenoir : ddiCapsRaven,
            .deviceId = NRed::callback->deviceID,
            .revision = NRed::callback->devRevision,
            .extRevision = static_cast<UInt32>(NRed::callback->enumRevision) + NRed::callback->devRevision,
            .pciRevision = NRed::callback->pciRevision,
        };
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("X6000FB", "Applied DDI Caps patches");

        if (ADDPR(debugEnabled)) {
            auto *logEnableMaskMinors =
                patcher.solveSymbol<void *>(id, "__ZN14AmdDalDmLogger19LogEnableMaskMinorsE", slide, size);
            patcher.clearError();

            if (logEnableMaskMinors == nullptr) {
                size_t offset = 0;
                PANIC_COND(!KernelPatcher::findPattern(kDalDmLoggerShouldLogPartialPattern,
                               kDalDmLoggerShouldLogPartialPatternMask, arrsize(kDalDmLoggerShouldLogPartialPattern),
                               reinterpret_cast<const void *>(slide), size, &offset),
                    "X6000FB", "Failed to solve LogEnableMaskMinors");
                auto *instAddr = reinterpret_cast<UInt8 *>(slide + offset);
                // inst + instSize + imm32 = addr
                logEnableMaskMinors = instAddr + 7 + *reinterpret_cast<SInt32 *>(instAddr + 3);
            }

            PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X6000FB",
                "Failed to enable kernel writing");
            memset(logEnableMaskMinors, 0xFF, 0x80);    // Enable all DalDmLogger logs
            MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

            // Enable all Display Core logs
            if (NRed::callback->attributes.isCatalina()) {
                const LookupPatchPlus patch = {&kextRadeonX6000Framebuffer, kInitPopulateDcInitDataCatalinaOriginal,
                    kInitPopulateDcInitDataCatalinaPatched, 1};
                PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB",
                    "Failed to apply populateDcInitData patch (10.15)");
            } else {
                const LookupPatchPlus patch = {&kextRadeonX6000Framebuffer, kInitPopulateDcInitDataOriginal,
                    kInitPopulateDcInitDataPatched, 1};
                PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB", "Failed to apply populateDcInitData patch");
            }

            const LookupPatchPlus patch = {&kextRadeonX6000Framebuffer, kBiosParserHelperInitWithDataOriginal,
                kBiosParserHelperInitWithDataPatched, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB",
                "Failed to apply AmdBiosParserHelper::initWithData patch");
        }

        return true;
    }

    return false;
}

UInt16 X6000FB::wrapGetEnumeratedRevision() { return NRed::callback->enumRevision; }

IOReturn X6000FB::wrapPopulateVramInfo(void *, void *fwInfo) {
    UInt32 channelCount = 1;
    auto *table = NRed::callback->getVBIOSDataTable<IGPSystemInfo>(0x1E);
    UInt8 memoryType = 0;
    if (table) {
        DBGLOG("X6000FB", "Fetching VRAM info from iGPU System Info");
        switch (table->header.formatRev) {
            case 1:
                switch (table->header.contentRev) {
                    case 11:
                    case 12:
                        if (table->infoV11.umaChannelCount) { channelCount = table->infoV11.umaChannelCount; }
                        memoryType = table->infoV11.memoryType;
                        break;
                    default:
                        DBGLOG("X6000FB", "Unsupported contentRev %d", table->header.contentRev);
                        break;
                }
                break;
            case 2:
                switch (table->header.contentRev) {
                    case 1:
                    case 2:
                        if (table->infoV2.umaChannelCount) { channelCount = table->infoV2.umaChannelCount; }
                        memoryType = table->infoV2.memoryType;
                        break;
                    default:
                        DBGLOG("X6000FB", "Unsupported contentRev %d", table->header.contentRev);
                        break;
                }
                break;
            default:
                DBGLOG("X6000FB", "Unsupported formatRev %d", table->header.formatRev);
                break;
        }
    } else {
        DBGLOG("X6000FB", "No iGPU System Info in Master Data Table");
    }
    auto &videoMemoryType = getMember<UInt32>(fwInfo, 0x1C);
    switch (memoryType) {
        case kDDR2MemType:
        case kDDR2FBDIMMMemType:
        case kLPDDR2MemType:
            videoMemoryType = kVideoMemoryTypeDDR2;
            break;
        case kDDR3MemType:
        case kLPDDR3MemType:
            videoMemoryType = kVideoMemoryTypeDDR3;
            break;
        case kDDR4MemType:
        case kLPDDR4MemType:
        case kDDR5MemType:    // AMD's Kexts don't know about DDR5
        case kLPDDR5MemType:
            videoMemoryType = kVideoMemoryTypeDDR4;
            break;
        default:
            DBGLOG("X6000FB", "Unsupported memory type %d. Assuming DDR4", memoryType);
            videoMemoryType = kVideoMemoryTypeDDR4;
            break;
    }
    getMember<UInt32>(fwInfo, 0x20) = channelCount * 64;    // VRAM Width (64-bit channels)
    return kIOReturnSuccess;
}

bool X6000FB::OnAppleBacklightDisplayLoad(void *, void *, IOService *newService, IONotifier *) {
    OSDictionary *params = OSDynamicCast(OSDictionary, newService->getProperty("IODisplayParameters"));
    if (params == nullptr) {
        DBGLOG("X6000FB", "%s: No 'IODisplayParameters' property", __FUNCTION__);
        return false;
    }

    OSDictionary *linearBrightness = OSDynamicCast(OSDictionary, params->getObject("linear-brightness"));
    if (linearBrightness == nullptr) {
        DBGLOG("X6000FB", "%s: No 'linear-brightness' property", __FUNCTION__);
        return false;
    }

    OSNumber *maxBrightness = OSDynamicCast(OSNumber, linearBrightness->getObject("max"));
    if (maxBrightness == nullptr) {
        DBGLOG("X6000FB", "%s: No 'max' property", __FUNCTION__);
        return false;
    }

    if (maxBrightness->unsigned32BitValue() == 0) {
        DBGLOG("X6000FB", "%s: 'max' property is 0", __FUNCTION__);
        return false;
    }

    callback->maxPwmBacklightLvl = maxBrightness->unsigned32BitValue();
    DBGLOG("X6000FB", "%s: Max brightness: 0x%X", __FUNCTION__, callback->maxPwmBacklightLvl);

    return true;
}

void X6000FB::registerDispMaxBrightnessNotif() {
    if (callback->dispNotif != nullptr) { return; }

    auto *matching = IOService::serviceMatching("AppleBacklightDisplay");
    if (matching == nullptr) {
        SYSLOG("X6000FB", "%s: Failed to create match dictionary", __FUNCTION__);
        return;
    }

    callback->dispNotif =
        IOService::addMatchingNotification(gIOFirstMatchNotification, matching, OnAppleBacklightDisplayLoad, nullptr);
    SYSLOG_COND(callback->dispNotif == nullptr, "X6000FB", "%s: Failed to register notification", __FUNCTION__);
    OSSafeReleaseNULL(matching);
}

IOReturn X6000FB::wrapSetAttributeForConnection(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute,
    uintptr_t value) {
    auto ret = FunctionCast(wrapSetAttributeForConnection, callback->orgSetAttributeForConnection)(framebuffer,
        connectIndex, attribute, value);
    if (attribute != FbAttributeBacklight) { return ret; }

    callback->curPwmBacklightLvl = static_cast<UInt32>(value);

    if ((NRed::callback->attributes.isBigSurAndLater() && NRed::callback->attributes.isRaven() &&
            callback->panelCntlPtr == nullptr) ||
        callback->embeddedPanelLink == nullptr) {
        return kIOReturnNoDevice;
    }

    if (callback->maxPwmBacklightLvl == 0) { return kIOReturnInternalError; }

    UInt32 percentage = callback->curPwmBacklightLvl * 100 / callback->maxPwmBacklightLvl;

    // AMDGPU doesn't use AUX on HDR/SDR displays that can use it. Why?
    if (callback->supportsAUX) {
        // TODO: Obtain the actual max brightness for the screen
        UInt32 auxValue = (callback->maxOLED * percentage) / 100;
        // dc_link_set_backlight_level_nits doesn't print the new backlight level, so we'll do it
        DBGLOG("X6000FB", "%s: New AUX brightness: %d millinits (%d nits)", __FUNCTION__, auxValue, (auxValue / 1000));
        callback->orgDcLinkSetBacklightLevelNits(callback->embeddedPanelLink, true, auxValue, 15000);
    } else if (NRed::callback->attributes.isBigSurAndLater() && NRed::callback->attributes.isRaven()) {
        // XX: Use the old brightness logic for now on Raven
        // until I can find out the actual problem with DMCU.
        UInt32 pwmValue = percentage >= 100 ? 0x1FF00 : ((percentage * 0xFF) / 100) << 8U;
        DBGLOG("X6000FB", "%s: New PWM brightness: 0x%X", __FUNCTION__, pwmValue);
        callback->orgDceDriverSetBacklight(callback->panelCntlPtr, pwmValue);
        return kIOReturnSuccess;
    } else {
        UInt32 pwmValue = (percentage * 0xFFFF) / 100;
        DBGLOG("X6000FB", "%s: New PWM brightness: 0x%X", __FUNCTION__, pwmValue);
        if (callback->orgDcLinkSetBacklightLevel(callback->embeddedPanelLink, pwmValue, 0)) { return kIOReturnSuccess; }
    }

    return kIOReturnDeviceError;
}

IOReturn X6000FB::wrapGetAttributeForConnection(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute,
    uintptr_t *value) {
    auto ret = FunctionCast(wrapGetAttributeForConnection, callback->orgGetAttributeForConnection)(framebuffer,
        connectIndex, attribute, value);
    if (attribute != FbAttributeBacklight) { return ret; }
    *value = callback->curPwmBacklightLvl;
    return kIOReturnSuccess;
}

UInt32 X6000FB::wrapGetNumberOfConnectors(void *that) {
    if (!callback->fixedVBIOS) {
        callback->fixedVBIOS = true;
        struct DispObjInfoTableV1_4 *objInfo = getMember<DispObjInfoTableV1_4 *>(that, 0x28);
        if (objInfo->formatRev == 1 && (objInfo->contentRev == 4 || objInfo->contentRev == 5)) {
            DBGLOG("X6000FB", "getNumberOfConnectors: Fixing VBIOS connectors");
            auto n = objInfo->pathCount;
            for (size_t i = 0, j = 0; i < n; i++) {
                // Skip invalid device tags
                if (objInfo->paths[i].devTag) {
                    objInfo->paths[j++] = objInfo->paths[i];
                } else {
                    objInfo->pathCount--;
                }
            }
        }
    }
    return FunctionCast(wrapGetNumberOfConnectors, callback->orgGetNumberOfConnectors)(that);
}

bool X6000FB::wrapIH40IVRingInitHardware(void *ctx, void *param2) {
    auto ret = FunctionCast(wrapIH40IVRingInitHardware, callback->orgIH40IVRingInitHardware)(ctx, param2);
    NRed::callback->writeReg32(mmIH_CHICKEN, NRed::callback->readReg32(mmIH_CHICKEN) | mmIH_MC_SPACE_GPA_ENABLE);
    return ret;
}

void X6000FB::wrapIRQMGRWriteRegister(void *ctx, UInt64 index, UInt32 value) {
    if (index == mmIH_CLK_CTRL) {
        if ((value & (1U << mmIH_DBUS_MUX_CLK_SOFT_OVERRIDE_SHIFT)) != 0) {
            value |= (1U << mmIH_IH_BUFFER_MEM_CLK_SOFT_OVERRIDE_SHIFT);
        }
    }
    FunctionCast(wrapIRQMGRWriteRegister, callback->orgIRQMGRWriteRegister)(ctx, index, value);
}

UInt32 X6000FB::wrapControllerPowerUp(void *that) {
    auto &m_flags = getMember<UInt8>(that, 0x5F18);
    auto send = (m_flags & 2) == 0;
    m_flags |= 4;    // All framebuffers enabled
    auto ret = FunctionCast(wrapControllerPowerUp, callback->orgControllerPowerUp)(that);
    if (send) { callback->orgMessageAccelerator(that, 0x1B, nullptr, nullptr, nullptr); }
    return ret;
}

void X6000FB::wrapDpReceiverPowerCtrl(void *link, bool power_on) {
    FunctionCast(wrapDpReceiverPowerCtrl, callback->orgDpReceiverPowerCtrl)(link, power_on);
    IOSleep(250);    // Link needs a bit of delay to change power state
}

UInt32 X6000FB::wrapDcePanelCntlHwInit(void *panelCntl) {
    callback->panelCntlPtr = panelCntl;
    return FunctionCast(wrapDcePanelCntlHwInit, callback->orgDcePanelCntlHwInit)(panelCntl);
}

void *X6000FB::wrapLinkCreate(void *data) {
    void *ret = FunctionCast(wrapLinkCreate, callback->orgLinkCreate)(data);

    if (ret == nullptr) { return nullptr; }

    auto signalType = getMember<UInt32>(ret, 0x38);
    switch (signalType) {
        case DC_SIGNAL_TYPE_LVDS: {
            if (callback->embeddedPanelLink != nullptr) {
                SYSLOG("X6000FB", "EMBEDDED PANEL LINK WAS ALREADY SET AND DISCOVERED NEW ONE!!!!");
                SYSLOG("X6000FB", "REPORT THIS TO THE DEVELOPERS AS SOON AS POSSIBLE!!!!");
            }
            callback->embeddedPanelLink = ret;
            DBGLOG("X6000FB", "Will use DMCU for display brightness control.");
        }
        case DC_SIGNAL_TYPE_EDP: {
            if (callback->embeddedPanelLink != nullptr) {
                SYSLOG("X6000FB", "EMBEDDED PANEL LINK WAS ALREADY SET AND DISCOVERED NEW ONE!!!!");
                SYSLOG("X6000FB", "REPORT THIS TO THE DEVELOPERS AS SOON AS POSSIBLE!!!!");
            }
            callback->embeddedPanelLink = ret;
            callback->supportsAUX = (callback->dcLinkCapsField.get(ret) & DC_DPCD_EXT_CAPS_OLED) != 0;

            DBGLOG("X6000FB", "Will use %s for display brightness control.", callback->supportsAUX ? "AUX" : "DMCU");
        }
        default: {
            break;
        }
    }

    return ret;
}

bool X6000FB::wrapInitWithPciInfo(void *that, void *pciDevice) {
    auto ret = FunctionCast(wrapInitWithPciInfo, callback->orgInitWithPciInfo)(that, pciDevice);
    getMember<UInt64>(that, 0x28) = 0xFFFFFFFFFFFFFFFF;    // Enable all log types
    getMember<UInt32>(that, 0x30) = 0xFF;                  // Enable all log severities
    return ret;
}

void X6000FB::wrapDoGPUPanic(void *, char const *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    auto *buf = static_cast<char *>(IOMalloc(1000));
    bzero(buf, 1000);
    vsnprintf(buf, 1000, fmt, va);
    va_end(va);

    DBGLOG("X6000FB", "doGPUPanic: %s", buf);
    IOSleep(10000);
    panic("%s", buf);
}

constexpr static const char *LogTypes[] = {
    "Error",
    "Warning",
    "Debug",
    "DC_Interface",
    "DTN",
    "Surface",
    "HW_Hotplug",
    "HW_LKTN",
    "HW_Mode",
    "HW_Resume",
    "HW_Audio",
    "HW_HPDIRQ",
    "MST",
    "Scaler",
    "BIOS",
    "BWCalcs",
    "BWValidation",
    "I2C_AUX",
    "Sync",
    "Backlight",
    "Override",
    "Edid",
    "DP_Caps",
    "Resource",
    "DML",
    "Mode",
    "Detect",
    "LKTN",
    "LinkLoss",
    "Underflow",
    "InterfaceTrace",
    "PerfTrace",
    "DisplayStats",
};

// Needed to prevent stack overflow
void X6000FB::wrapDmLoggerWrite(void *, const UInt32 logType, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    auto *message = static_cast<char *>(IOMalloc(0x1000));
    vsnprintf(message, 0x1000, fmt, va);
    va_end(va);
    auto *epilogue = message[strnlen(message, 0x1000) - 1] == '\n' ? "" : "\n";
    if (logType < arrsize(LogTypes)) {
        kprintf("[%s]\t%s%s", LogTypes[logType], message, epilogue);
    } else {
        kprintf("%s%s", message, epilogue);
    }
    IOFree(message, 0x1000);
}
