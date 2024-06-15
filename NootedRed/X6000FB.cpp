//! Copyright © 2022-2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "X6000FB.hpp"
#include "ATOMBIOS.hpp"
#include "NRed.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>

static const char *pathRadeonX6000Framebuffer =
    "/System/Library/Extensions/AMDRadeonX6000Framebuffer.kext/Contents/MacOS/AMDRadeonX6000Framebuffer";

static KernelPatcher::KextInfo kextRadeonX6000Framebuffer {"com.apple.kext.AMDRadeonX6000Framebuffer",
    &pathRadeonX6000Framebuffer, 1, {}, {}, KernelPatcher::KextInfo::Unloaded};

X6000FB *X6000FB::callback = nullptr;

void X6000FB::init() {
    callback = this;
    lilu.onKextLoadForce(&kextRadeonX6000Framebuffer);
}

bool X6000FB::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX6000Framebuffer.loadIndex == id) {
        NRed::callback->setRMMIOIfNecessary();

        CAILAsicCapsEntry *orgAsicCapsTable = nullptr;
        SolveRequestPlus solveRequest {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable, kCailAsicCapsTablePattern};
        PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "X6000FB", "Failed to resolve CAIL_ASIC_CAPS_TABLE");

        bool sonoma144 = getKernelVersion() > KernelVersion::Sonoma ||
                         (getKernelVersion() == KernelVersion::Sonoma && getKernelMinorVersion() >= 4);

        if (NRed::callback->enableBacklight) {
            if (sonoma144) {
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

        bool ventura = getKernelVersion() >= KernelVersion::Ventura;
        if (ventura) {
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
            if (getKernelVersion() > KernelVersion::Sonoma ||
                (getKernelVersion() == KernelVersion::Sonoma && getKernelMinorVersion() >= 4)) {
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

        bool renoir = NRed::callback->chipType >= ChipType::Renoir;
        if (renoir) {
            RouteRequestPlus request {"_IH_4_0_IVRing_InitHardware", wrapIH40IVRingInitHardware,
                this->orgIH40IVRingInitHardware, kIH40IVRingInitHardwarePattern, kIH40IVRingInitHardwareMask};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB",
                "Failed to route IH_4_0_IVRing_InitHardware");
            if (getKernelVersion() > KernelVersion::Sonoma ||
                (getKernelVersion() == KernelVersion::Sonoma && getKernelMinorVersion() >= 4)) {
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

        if (ventura) {
            RouteRequestPlus request {"__ZN34AMDRadeonX6000_AmdRadeonController7powerUpEv", wrapControllerPowerUp,
                this->orgControllerPowerUp};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB", "Failed to route powerUp");
        }

        if (NRed::callback->enableBacklight) {
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

        if (sonoma144) {
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

        if (getKernelVersion() == KernelVersion::Catalina) {
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

        if (ventura) {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX6000Framebuffer, kControllerPowerUpOriginal, kControllerPowerUpOriginalMask,
                    kControllerPowerUpReplace, kControllerPowerUpReplaceMask, 1},
                {&kextRadeonX6000Framebuffer, kValidateDetailedTimingOriginal, kValidateDetailedTimingPatched, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "X6000FB",
                "Failed to apply logic revert patches");
        }

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "x5000",
            "Failed to enable kernel writing");
        *orgAsicCapsTable = {
            .familyId = AMDGPU_FAMILY_RAVEN,
            .caps = renoir ? ddiCapsRenoir : ddiCapsRaven,
            .deviceId = NRed::callback->deviceId,
            .revision = NRed::callback->revision,
            .extRevision = static_cast<UInt32>(NRed::callback->enumRevision) + NRed::callback->revision,
            .pciRevision = NRed::callback->pciRevision,
        };
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("X6000FB", "Applied DDI Caps patches");

        this->biosScratchReg = NRed::callback->getVBIOSDataTable<AtomFirmwareInfo>(0x4)->biosScratchRegStartAddr;
        DBGLOG("X6000FB", "BIOS Scratch Reg = 0x%X", this->biosScratchReg);

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
                        [[fallthrough]];
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
                        [[fallthrough]];
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
            [[fallthrough]];
        case kDDR2FBDIMMMemType:
            [[fallthrough]];
        case kLPDDR2MemType:
            videoMemoryType = kVideoMemoryTypeDDR2;
            break;
        case kDDR3MemType:
            [[fallthrough]];
        case kLPDDR3MemType:
            videoMemoryType = kVideoMemoryTypeDDR3;
            break;
        case kDDR4MemType:
            [[fallthrough]];
        case kLPDDR4MemType:
            [[fallthrough]];
        case kDDR5MemType:    //! AMD's Kexts don't know about DDR5
            [[fallthrough]];
        case kLPDDR5MemType:
            videoMemoryType = kVideoMemoryTypeDDR4;
            break;
        default:
            DBGLOG("X6000FB", "Unsupported memory type %d. Assuming DDR4", memoryType);
            videoMemoryType = kVideoMemoryTypeDDR4;
            break;
    }
    getMember<UInt32>(fwInfo, 0x20) = channelCount * 64;    //! VRAM Width (64-bit channels)
    return kIOReturnSuccess;
}

bool X6000FB::OnAppleBacklightDisplayLoad(void *, void *, IOService *newService, IONotifier *) {
    OSDictionary *params = OSDynamicCast(OSDictionary, newService->getProperty("IODisplayParameters"));
    if (!params) {
        DBGLOG("X6000FB", "OnAppleBacklightDisplayLoad: No 'IODisplayParameters' property");
        return false;
    }

    OSDictionary *linearBrightness = OSDynamicCast(OSDictionary, params->getObject("linear-brightness"));
    if (!linearBrightness) {
        DBGLOG("X6000FB", "OnAppleBacklightDisplayLoad: No 'linear-brightness' property");
        return false;
    }

    OSNumber *maxBrightness = OSDynamicCast(OSNumber, linearBrightness->getObject("max"));
    if (!maxBrightness) {
        DBGLOG("X6000FB", "OnAppleBacklightDisplayLoad: No 'max' property");
        return false;
    }

    if (!maxBrightness->unsigned32BitValue()) {
        DBGLOG("X6000FB", "OnAppleBacklightDisplayLoad: 'max' property is 0");
        return false;
    }

    callback->maxPwmBacklightLvl = maxBrightness->unsigned32BitValue();
    DBGLOG("X6000FB", "OnAppleBacklightDisplayLoad: Max brightness: 0x%X", callback->maxPwmBacklightLvl);

    return true;
}

void X6000FB::registerDispMaxBrightnessNotif() {
    if (callback->dispNotif) { return; }

    auto *matching = IOService::serviceMatching("AppleBacklightDisplay");
    if (!matching) {
        SYSLOG("X6000FB", "registerDispMaxBrightnessNotif: Failed to create match dictionary");
        return;
    }

    callback->dispNotif =
        IOService::addMatchingNotification(gIOFirstMatchNotification, matching, OnAppleBacklightDisplayLoad, nullptr);
    SYSLOG_COND(!callback->dispNotif, "X6000FB", "registerDispMaxBrightnessNotif: Failed to register notification");
    OSSafeReleaseNULL(matching);
}

IOReturn X6000FB::wrapSetAttributeForConnection(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute,
    uintptr_t value) {
    auto ret = FunctionCast(wrapSetAttributeForConnection, callback->orgSetAttributeForConnection)(framebuffer,
        connectIndex, attribute, value);
    if (attribute != static_cast<UInt32>('bklt')) { return ret; }

    if (!callback->maxPwmBacklightLvl) {
        DBGLOG("X6000FB", "setAttributeForConnection: May not control backlight at this time; maxPwmBacklightLvl is 0");
        return kIOReturnSuccess;
    }

    if (callback->embeddedPanelLink == nullptr) {
        DBGLOG("X6000FB",
            "setAttributeForConnection: May not control backight at this time; embeddedPanelLink is nullptr");
        return kIOReturnSuccess;
    }

    //! Set the backlight
    callback->curPwmBacklightLvl = static_cast<UInt32>(value);
    UInt32 percentage = callback->curPwmBacklightLvl * 100 / callback->maxPwmBacklightLvl;

    bool success;

    //! AMDGPU doesn't use AUX on HDR/SDR displays that can use it. Why?
    if (callback->supportsAUX) {
        //! TODO: Obtain the actual max brightness for the screen
        UInt32 auxValue = (callback->maxOLED * percentage) / 100;
        //! dc_link_set_backlight_level_nits doesn't print the new backlight level, so we'll do it
        DBGLOG("X6000FB", "setAttributeForConnection: New AUX brightness: %d millinits (%d nits)", auxValue,
            (auxValue / 1000));
        //! Save the brightness value so the driver can restore it on its own on some specific occasions.
        //! For instance, when waking from sleep.
        NRed::callback->writeReg32(callback->biosScratchReg + 2, auxValue);
        success = callback->orgDcLinkSetBacklightLevelNits(callback->embeddedPanelLink, true, auxValue, 15000);
    } else {
        UInt32 pwmValue = percentage >= 100 ? 0x1FF00 : ((percentage * 0xFF) / 100) << 8U;
        DBGLOG("X6000FB", "setAttributeForConnection: New PWM brightness: 0x%X", pwmValue);
        //! Ditto.
        NRed::callback->writeReg32(callback->biosScratchReg + 2, pwmValue);
        success = callback->orgDcLinkSetBacklightLevel(callback->embeddedPanelLink, pwmValue, 0);
    }

    if (success) {
        return kIOReturnSuccess;
    } else {
        DBGLOG("X6000FB", "Failed to set backlight level");
        return kIOReturnDeviceError;
    }
}

IOReturn X6000FB::wrapGetAttributeForConnection(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute,
    uintptr_t *value) {
    auto ret = FunctionCast(wrapGetAttributeForConnection, callback->orgGetAttributeForConnection)(framebuffer,
        connectIndex, attribute, value);
    if (attribute != static_cast<UInt32>('bklt')) { return ret; }
    *value = callback->curPwmBacklightLvl;
    return kIOReturnSuccess;
}

UInt32 X6000FB::wrapGetNumberOfConnectors(void *that) {
    static bool once = false;
    if (!once) {
        once = true;
        struct DispObjInfoTableV1_4 *objInfo = getMember<DispObjInfoTableV1_4 *>(that, 0x28);
        if (objInfo->formatRev == 1 && (objInfo->contentRev == 4 || objInfo->contentRev == 5)) {
            DBGLOG("X6000FB", "getNumberOfConnectors: Fixing VBIOS connectors");
            auto n = objInfo->pathCount;
            for (size_t i = 0, j = 0; i < n; i++) {
                //! Skip invalid device tags
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
        value |= (value & (1U << mmIH_DBUS_MUX_CLK_SOFT_OVERRIDE_SHIFT)) >>
                 (mmIH_DBUS_MUX_CLK_SOFT_OVERRIDE_SHIFT - mmIH_IH_BUFFER_MEM_CLK_SOFT_OVERRIDE_SHIFT);
        DBGLOG("X6000FB", "IRQMGR_WriteRegister: Set IH_BUFFER_MEM_CLK_SOFT_OVERRIDE");
    }
    FunctionCast(wrapIRQMGRWriteRegister, callback->orgIRQMGRWriteRegister)(ctx, index, value);
}

UInt32 X6000FB::wrapControllerPowerUp(void *that) {
    auto &m_flags = getMember<UInt8>(that, 0x5F18);
    auto send = !(m_flags & 2);
    m_flags |= 4;    //! All framebuffers enabled
    auto ret = FunctionCast(wrapControllerPowerUp, callback->orgControllerPowerUp)(that);
    if (send) { callback->orgMessageAccelerator(that, 0x1B, nullptr, nullptr, nullptr); }
    return ret;
}

void X6000FB::wrapDpReceiverPowerCtrl(void *link, bool power_on) {
    FunctionCast(wrapDpReceiverPowerCtrl, callback->orgDpReceiverPowerCtrl)(link, power_on);
    IOSleep(250);    //! Link needs a bit of delay to change power state
}

void *X6000FB::wrapLinkCreate(void *data) {
    void *ret = FunctionCast(wrapLinkCreate, callback->orgLinkCreate)(data);

    if (!ret) { return nullptr; }

    auto signalType = getMember<UInt32>(ret, 0x38);
    switch (signalType) {
        case DC_SIGNAL_TYPE_LVDS: {
            callback->embeddedPanelLink = ret;
            callback->supportsAUX = false;
            DBGLOG("X6000FB", "Will use DMCU for display brightness control.");
        }
        case DC_SIGNAL_TYPE_EDP: {
            callback->embeddedPanelLink = ret;
            UInt32 fieldBase;
            switch (getKernelVersion()) {
                case KernelVersion::Catalina:
                    fieldBase = 0x1EA;
                    break;
                case KernelVersion::BigSur:
                    fieldBase = 0x26C;
                    break;
                case KernelVersion::Monterey:
                    fieldBase = 0x284;
                    break;
                case KernelVersion::Ventura... KernelVersion::Sequoia:
                    fieldBase = 0x28C;
                    break;
                default:
                    PANIC("X6000FB", "Unsupported kernel version %d", getKernelVersion());
            }
            callback->supportsAUX = (getMember<UInt8>(ret, fieldBase) & DC_DPCD_EXT_CAPS_OLED) != 0;

            DBGLOG("X6000FB", "Will use %s for display brightness control.", callback->supportsAUX ? "AUX" : "DMCU");
        }
    }

    return ret;
}
