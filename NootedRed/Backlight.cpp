// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>
#include <PrivateHeaders/Backlight.hpp>
#include <PrivateHeaders/GPUDriversAMD/DC/DPCD.hpp>
#include <PrivateHeaders/GPUDriversAMD/DC/SignalTypes.hpp>
#include <PrivateHeaders/NRed.hpp>
#include <PrivateHeaders/PatcherPlus.hpp>

//------ Target Kexts ------//

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

static const char *pathBacklight = "/System/Library/Extensions/AppleBacklight.kext/Contents/MacOS/AppleBacklight";
static const char *pathMCCSControl = "/System/Library/Extensions/AppleMCCSControl.kext/Contents/MacOS/AppleMCCSControl";

static KernelPatcher::KextInfo kextBacklight {
    "com.apple.driver.AppleBacklight",
    &pathBacklight,
    1,
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};
static KernelPatcher::KextInfo kextMCCSControl {
    "com.apple.driver.AppleMCCSControl",
    &pathMCCSControl,
    1,
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

//------ Patterns ------//

static const UInt8 kDcePanelCntlHwInitPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41,
    0x54, 0x53, 0x50, 0x49, 0x89, 0xFD, 0x4C, 0x8D, 0x45, 0xD4, 0x41, 0xC7, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kDcePanelCntlHwInitPattern1404[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54, 0x53,
    0x48, 0x83, 0xEC, 0x10, 0x48, 0x89, 0xFB, 0x4C, 0x8D, 0x45, 0xDC, 0x41, 0xC7, 0x00, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kDceDriverSetBacklightPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41,
    0x54, 0x53, 0x50, 0x41, 0x89, 0xF0, 0x40, 0x89, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x7F, 0x08, 0x40, 0x8B, 0x40, 0x28, 0x8B, 0x70, 0x10};
static const UInt8 kDceDriverSetBacklightPatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF,
    0xFF};

static const UInt8 kLinkCreatePattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54, 0x53,
    0x48, 0x81, 0xEC, 0x00, 0x03, 0x00, 0x00, 0x49, 0x89, 0xFD, 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B,
    0x00, 0x48, 0x00, 0x00, 0x00, 0xBF, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kLinkCreatePatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kDcLinkSetBacklightLevelPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55,
    0x41, 0x54, 0x53, 0x50, 0x41, 0x89, 0xD6, 0x41, 0x89, 0xF4};
static const UInt8 kDcLinkSetBacklightLevelPattern1404[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55,
    0x41, 0x54, 0x53, 0x50, 0x89, 0xD3, 0x41, 0x89, 0xF6, 0x49, 0x89, 0xFC};

static const UInt8 kDcLinkSetBacklightLevelNitsPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x53, 0x50, 0x40, 0x88, 0x75, 0x00,
    0x48, 0x85, 0xFF, 0x74, 0x00};
static const UInt8 kDcLinkSetBacklightLevelNitsPatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};

//------ Module Logic ------//

constexpr UInt32 FbAttributeBacklight = static_cast<UInt32>('bklt');

static Backlight instance {};

Backlight &Backlight::singleton() { return instance; }

void Backlight::init() {
    PANIC_COND(this->initialised, "Backlight", "Attempted to initialise module twice!");
    this->initialised = true;

    bool bkltArg = false;
    if (BaseDeviceInfo::get().modelType != WIOKit::ComputerModel::ComputerLaptop &&
        (!PE_parse_boot_argn("AMDBacklight", &bkltArg, sizeof(bkltArg)) || !bkltArg)) {
        SYSLOG("Backlight", "Module disabled.");
        return;
    }

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
        case KernelVersion::Tahoe:
            this->dcLinkCapsField = 0x28C;
            break;
        default:
            PANIC("Backlight", "Unsupported kernel version %d", getKernelVersion());
    }

    SYSLOG("Backlight", "Module initialised.");

    lilu.onKextLoadForce(&kextRadeonX6000Framebuffer);
    lilu.onKextLoadForce(&kextBacklight);
    lilu.onKextLoadForce(&kextMCCSControl);
    lilu.onPatcherLoadForce(
        [](void *user, KernelPatcher &) { static_cast<Backlight *>(user)->registerDispMaxBrightnessNotif(); }, this);
    lilu.onKextLoadForce(
        nullptr, 0,
        [](void *user, KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
            static_cast<Backlight *>(user)->processKext(patcher, id, slide, size);
        },
        this);
}

void Backlight::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX6000Framebuffer.loadIndex == id) {
        if (NRed::singleton().getAttributes().isBigSurAndLater() && NRed::singleton().getAttributes().isRaven()) {
            PatcherPlus::PatternSolveRequest solveRequest {"_dce_driver_set_backlight", this->orgDceDriverSetBacklight,
                kDceDriverSetBacklightPattern, kDceDriverSetBacklightPatternMask};
            PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "Backlight",
                "Failed to resolve dce_driver_set_backlight");
        }
        if (NRed::singleton().getAttributes().isSonoma1404AndLater()) {
            PatcherPlus::PatternSolveRequest solveRequest {"_dc_link_set_backlight_level",
                this->orgDcLinkSetBacklightLevel, kDcLinkSetBacklightLevelPattern1404};
            PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "Backlight",
                "Failed to resolve dc_link_set_backlight_level");
        } else {
            PatcherPlus::PatternSolveRequest solveRequest {"_dc_link_set_backlight_level",
                this->orgDcLinkSetBacklightLevel, kDcLinkSetBacklightLevelPattern};
            PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "Backlight",
                "Failed to resolve dc_link_set_backlight_level");
        }

        PatcherPlus::PatternSolveRequest solveRequest {"_dc_link_set_backlight_level_nits",
            this->orgDcLinkSetBacklightLevelNits, kDcLinkSetBacklightLevelNitsPattern,
            kDcLinkSetBacklightLevelNitsPatternMask};
        PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "Backlight",
            "Failed to resolve dc_link_set_backlight_level_nits");
        if (NRed::singleton().getAttributes().isBigSurAndLater() && NRed::singleton().getAttributes().isRaven()) {
            if (NRed::singleton().getAttributes().isSonoma1404AndLater()) {
                PatcherPlus::PatternRouteRequest request {"_dce_panel_cntl_hw_init", wrapDcePanelCntlHwInit,
                    this->orgDcePanelCntlHwInit, kDcePanelCntlHwInitPattern1404};
                PANIC_COND(!request.route(patcher, id, slide, size), "Backlight",
                    "Failed to route dce_panel_cntl_hw_init (14.4+)");
            } else {
                PatcherPlus::PatternRouteRequest request {"_dce_panel_cntl_hw_init", wrapDcePanelCntlHwInit,
                    this->orgDcePanelCntlHwInit, kDcePanelCntlHwInitPattern};
                PANIC_COND(!request.route(patcher, id, slide, size), "Backlight",
                    "Failed to route dce_panel_cntl_hw_init");
            }
        }
        PatcherPlus::PatternRouteRequest requests[] = {
            {"_link_create", wrapLinkCreate, this->orgLinkCreate, kLinkCreatePattern, kLinkCreatePatternMask},
            {"__ZN35AMDRadeonX6000_AmdRadeonFramebuffer25setAttributeForConnectionEijm", wrapSetAttributeForConnection,
                this->orgSetAttributeForConnection},
            {"__ZN35AMDRadeonX6000_AmdRadeonFramebuffer25getAttributeForConnectionEijPm", wrapGetAttributeForConnection,
                this->orgGetAttributeForConnection},
        };
        PANIC_COND(!PatcherPlus::PatternRouteRequest::routeAll(patcher, id, requests, slide, size), "Backlight",
            "Failed to route backlight symbols");
    } else if (kextBacklight.loadIndex == id) {
        KernelPatcher::RouteRequest request {"__ZN15AppleIntelPanel10setDisplayEP9IODisplay", wrapApplePanelSetDisplay,
            orgApplePanelSetDisplay};
        if (patcher.routeMultiple(kextBacklight.loadIndex, &request, 1, slide, size)) {
            static const UInt8 find[] = "F%uT%04x";
            static const UInt8 replace[] = "F%uTxxxx";
            const PatcherPlus::MaskedLookupPatch patch {&kextBacklight, find, replace, 1};
            SYSLOG_COND(!patch.apply(patcher, slide, size), "Backlight", "Failed to apply backlight patch");
        }
        patcher.clearError();
    } else if (kextMCCSControl.loadIndex == id) {
        KernelPatcher::RouteRequest requests[] = {
            {"__ZN25AppleMCCSControlGibraltar5probeEP9IOServicePi", returnZero},
            {"__ZN21AppleMCCSControlCello5probeEP9IOServicePi", returnZero},
        };
        patcher.routeMultiple(id, requests, slide, size);
        patcher.clearError();
    }
}

bool Backlight::OnAppleBacklightDisplayLoad(void *, void *, IOService *newService, IONotifier *) {
    OSDictionary *params = OSDynamicCast(OSDictionary, newService->getProperty("IODisplayParameters"));
    if (params == nullptr) {
        DBGLOG("Backlight", "%s: No 'IODisplayParameters' property", __FUNCTION__);
        return false;
    }

    OSDictionary *linearBrightness = OSDynamicCast(OSDictionary, params->getObject("linear-brightness"));
    if (linearBrightness == nullptr) {
        DBGLOG("Backlight", "%s: No 'linear-brightness' property", __FUNCTION__);
        return false;
    }

    OSNumber *maxBrightness = OSDynamicCast(OSNumber, linearBrightness->getObject("max"));
    if (maxBrightness == nullptr) {
        DBGLOG("Backlight", "%s: No 'max' property", __FUNCTION__);
        return false;
    }

    if (maxBrightness->unsigned32BitValue() == 0) {
        DBGLOG("Backlight", "%s: 'max' property is 0", __FUNCTION__);
        return false;
    }

    singleton().maxPwmBacklightLvl = maxBrightness->unsigned32BitValue();
    DBGLOG("Backlight", "%s: Max brightness: 0x%X", __FUNCTION__, singleton().maxPwmBacklightLvl);

    return true;
}

void Backlight::registerDispMaxBrightnessNotif() {
    if (this->dispNotif != nullptr) { return; }

    auto *matching = IOService::serviceMatching("AppleBacklightDisplay");
    if (matching == nullptr) {
        SYSLOG("Backlight", "%s: Failed to create match dictionary", __FUNCTION__);
        return;
    }

    this->dispNotif =
        IOService::addMatchingNotification(gIOFirstMatchNotification, matching, OnAppleBacklightDisplayLoad, nullptr);
    SYSLOG_COND(this->dispNotif == nullptr, "Backlight", "%s: Failed to register notification", __FUNCTION__);
    OSSafeReleaseNULL(matching);
}

IOReturn Backlight::wrapSetAttributeForConnection(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute,
    uintptr_t value) {
    auto ret = FunctionCast(wrapSetAttributeForConnection, singleton().orgSetAttributeForConnection)(framebuffer,
        connectIndex, attribute, value);
    if (attribute != FbAttributeBacklight) { return ret; }

    singleton().curPwmBacklightLvl = static_cast<UInt32>(value);

    if ((NRed::singleton().getAttributes().isBigSurAndLater() && NRed::singleton().getAttributes().isRaven() &&
            singleton().panelCntlPtr == nullptr) ||
        singleton().embeddedPanelLink == nullptr) {
        return kIOReturnNoDevice;
    }

    if (singleton().maxPwmBacklightLvl == 0) { return kIOReturnInternalError; }

    UInt32 percentage = singleton().curPwmBacklightLvl * 100 / singleton().maxPwmBacklightLvl;

    if (singleton().supportsAUX) {
        // TODO: Obtain the actual max brightness for the screen
        UInt32 auxValue = (singleton().maxOLED * percentage) / 100;
        // dc_link_set_backlight_level_nits doesn't print the new backlight level, so we'll do it
        DBGLOG("Backlight", "%s: New AUX brightness: %d millinits (%d nits)", __FUNCTION__, auxValue,
            (auxValue / 1000));
        singleton().orgDcLinkSetBacklightLevelNits(singleton().embeddedPanelLink, true, auxValue, 15000);
    } else if (NRed::singleton().getAttributes().isBigSurAndLater() && NRed::singleton().getAttributes().isRaven()) {
        // XX: Use the old brightness logic for now on Raven
        // until I can find out the actual problem with DMCU.
        UInt32 pwmValue = percentage >= 100 ? 0x1FF00 : ((percentage * 0xFF) / 100) << 8U;
        DBGLOG("Backlight", "%s: New PWM brightness: 0x%X", __FUNCTION__, pwmValue);
        singleton().orgDceDriverSetBacklight(singleton().panelCntlPtr, pwmValue);
        return kIOReturnSuccess;
    } else {
        UInt32 pwmValue = (percentage * 0xFFFF) / 100;
        DBGLOG("Backlight", "%s: New PWM brightness: 0x%X", __FUNCTION__, pwmValue);
        if (singleton().orgDcLinkSetBacklightLevel(singleton().embeddedPanelLink, pwmValue, 0)) {
            return kIOReturnSuccess;
        }
    }

    return kIOReturnDeviceError;
}

IOReturn Backlight::wrapGetAttributeForConnection(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute,
    uintptr_t *value) {
    auto ret = FunctionCast(wrapGetAttributeForConnection, singleton().orgGetAttributeForConnection)(framebuffer,
        connectIndex, attribute, value);
    if (attribute != FbAttributeBacklight) { return ret; }
    *value = singleton().curPwmBacklightLvl;
    return kIOReturnSuccess;
}

UInt32 Backlight::wrapDcePanelCntlHwInit(void *panelCntl) {
    singleton().panelCntlPtr = panelCntl;
    return FunctionCast(wrapDcePanelCntlHwInit, singleton().orgDcePanelCntlHwInit)(panelCntl);
}

void *Backlight::wrapLinkCreate(void *data) {
    void *ret = FunctionCast(wrapLinkCreate, singleton().orgLinkCreate)(data);

    if (ret == nullptr) { return nullptr; }

    auto signalType = getMember<UInt32>(ret, 0x38);
    switch (signalType) {
        case SIGNAL_TYPE_LVDS: {
            if (singleton().embeddedPanelLink != nullptr) {
                SYSLOG("Backlight", "EMBEDDED PANEL LINK WAS ALREADY SET AND DISCOVERED NEW ONE!!!!");
                SYSLOG("Backlight", "REPORT THIS TO THE DEVELOPERS AS SOON AS POSSIBLE!!!!");
            }
            singleton().embeddedPanelLink = ret;
            DBGLOG("Backlight", "Will use DMCU for display brightness control.");
        }
        case SIGNAL_TYPE_EDP: {
            if (singleton().embeddedPanelLink != nullptr) {
                SYSLOG("Backlight", "EMBEDDED PANEL LINK WAS ALREADY SET AND DISCOVERED NEW ONE!!!!");
                SYSLOG("Backlight", "REPORT THIS TO THE DEVELOPERS AS SOON AS POSSIBLE!!!!");
            }
            singleton().embeddedPanelLink = ret;
            singleton().supportsAUX = (singleton().dcLinkCapsField.get(ret) & DC_DPCD_EXT_CAPS_OLED) != 0;

            DBGLOG("Backlight", "Will use %s for display brightness control.",
                singleton().supportsAUX ? "AUX" : "DMCU");
        }
        default: {
            break;
        }
    }

    return ret;
}

struct ApplePanelData {
    const char *deviceName;
    UInt8 deviceData[36];
};

static ApplePanelData appleBacklightData[] = {
    {"F14Txxxx", {0x00, 0x11, 0x00, 0x00, 0x00, 0x34, 0x00, 0x52, 0x00, 0x73, 0x00, 0x94, 0x00, 0xBE, 0x00, 0xFA, 0x01,
                     0x36, 0x01, 0x72, 0x01, 0xC5, 0x02, 0x2F, 0x02, 0xB9, 0x03, 0x60, 0x04, 0x1A, 0x05, 0x0A, 0x06,
                     0x0E, 0x07, 0x10}},
    {"F15Txxxx", {0x00, 0x11, 0x00, 0x00, 0x00, 0x36, 0x00, 0x54, 0x00, 0x7D, 0x00, 0xB2, 0x00, 0xF5, 0x01, 0x49, 0x01,
                     0xB1, 0x02, 0x2B, 0x02, 0xB8, 0x03, 0x59, 0x04, 0x13, 0x04, 0xEC, 0x05, 0xF3, 0x07, 0x34, 0x08,
                     0xAF, 0x0A, 0xD9}},
    {"F16Txxxx", {0x00, 0x11, 0x00, 0x00, 0x00, 0x18, 0x00, 0x27, 0x00, 0x3A, 0x00, 0x52, 0x00, 0x71, 0x00, 0x96, 0x00,
                     0xC4, 0x00, 0xFC, 0x01, 0x40, 0x01, 0x93, 0x01, 0xF6, 0x02, 0x6E, 0x02, 0xFE, 0x03, 0xAA, 0x04,
                     0x78, 0x05, 0x6C}},
    {"F17Txxxx", {0x00, 0x11, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x34, 0x00, 0x4F, 0x00, 0x71, 0x00, 0x9B, 0x00, 0xCF, 0x01,
                     0x0E, 0x01, 0x5D, 0x01, 0xBB, 0x02, 0x2F, 0x02, 0xB9, 0x03, 0x60, 0x04, 0x29, 0x05, 0x1E, 0x06,
                     0x44, 0x07, 0xA1}},
    {"F18Txxxx", {0x00, 0x11, 0x00, 0x00, 0x00, 0x53, 0x00, 0x8C, 0x00, 0xD5, 0x01, 0x31, 0x01, 0xA2, 0x02, 0x2E, 0x02,
                     0xD8, 0x03, 0xAE, 0x04, 0xAC, 0x05, 0xE5, 0x07, 0x59, 0x09, 0x1C, 0x0B, 0x3B, 0x0D, 0xD0, 0x10,
                     0xEA, 0x14, 0x99}},
    {"F19Txxxx", {0x00, 0x11, 0x00, 0x00, 0x02, 0x8F, 0x03, 0x53, 0x04, 0x5A, 0x05, 0xA1, 0x07, 0xAE, 0x0A, 0x3D, 0x0E,
                     0x14, 0x13, 0x74, 0x1A, 0x5E, 0x24, 0x18, 0x31, 0xA9, 0x44, 0x59, 0x5E, 0x76, 0x83, 0x11, 0xB6,
                     0xC7, 0xFF, 0x7B}},
    {"F24Txxxx", {0x00, 0x11, 0x00, 0x01, 0x00, 0x34, 0x00, 0x52, 0x00, 0x73, 0x00, 0x94, 0x00, 0xBE, 0x00, 0xFA, 0x01,
                     0x36, 0x01, 0x72, 0x01, 0xC5, 0x02, 0x2F, 0x02, 0xB9, 0x03, 0x60, 0x04, 0x1A, 0x05, 0x0A, 0x06,
                     0x0E, 0x07, 0x10}},
};

size_t Backlight::returnZero() { return 0; }

bool Backlight::wrapApplePanelSetDisplay(IOService *that, IODisplay *display) {
    static bool once = false;
    if (!once) {
        once = true;
        auto *panels = OSDynamicCast(OSDictionary, that->getProperty("ApplePanels"));
        if (panels) {
            auto *rawPanels = panels->copyCollection();
            panels = OSDynamicCast(OSDictionary, rawPanels);

            if (panels) {
                for (auto &entry : appleBacklightData) {
                    auto pd = OSData::withBytes(entry.deviceData, sizeof(entry.deviceData));
                    if (pd) {
                        panels->setObject(entry.deviceName, pd);
                        // No release required by current AppleBacklight implementation.
                    } else {
                        SYSLOG("Backlight", "setDisplay: Cannot allocate data for %s", entry.deviceName);
                    }
                }
                that->setProperty("ApplePanels", panels);
            }

            OSSafeReleaseNULL(rawPanels);
        } else {
            SYSLOG("Backlight", "setDisplay: Missing ApplePanels property");
        }
    }

    bool ret = FunctionCast(wrapApplePanelSetDisplay, singleton().orgApplePanelSetDisplay)(that, display);
    DBGLOG("Backlight", "setDisplay >> %d", ret);
    return ret;
}
