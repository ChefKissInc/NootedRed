// AMD Display Backlight
// TODO: Get rid of this absolute garbage.
//
// Copyright (C) 2024-2026  ChefKiss.
//
// Licensed under the ChefKiss Open Knowledge License version 1.
//
// This work is licensed to you under the terms of the ChefKiss Open Knowledge
// License, either version 1 of the License, or (at your option) any later
// version published by ChefKiss.
//
// This work may not be used to train or develop artificial intelligence,
// and may not be used with artificial intelligence in any other way. See
// the ChefKiss Open Knowledge License for details.
//
// This work is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// ChefKiss Open Knowledge License for more details.
//
// You should have received a copy of the ChefKiss Open Knowledge License
// along with this work.  If not, see https://git.chefkiss.dev/ChefKiss/OpenKnowledge/raw/branch/master/LICENSE-v1.md.

#include <Backlight.hpp>
#include <GPUDriversAMD/DC/SignalTypes.hpp>
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_iokit.hpp>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>
#include <IOKit/IONotifier.h>
#include <IOKit/IOReturn.h>
#include <IOKit/IOService.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/graphics/IOGraphicsTypes.h>
#include <Kexts.hpp>
#include <NRed.hpp>
#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/PatcherPlus.hpp>
#include <libkern/OSTypes.h>
#include <libkern/c++/OSData.h>
#include <libkern/c++/OSDictionary.h>
#include <libkern/c++/OSMetaClass.h>
#include <libkern/c++/OSNumber.h>
#include <mach/i386/vm_types.h>
#include <pexpert/pexpert.h>

static const UInt8 kLinkCreatePattern[]     = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                                               0x53, 0x48, 0x81, 0xEC, 0x00, 0x03, 0x00, 0x00, 0x49, 0x89, 0xFD, 0x48,
                                               0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x00, 0x48, 0x00, 0x00,
                                               0x00, 0xBF, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kLinkCreatePatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                               0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                                               0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00,
                                               0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kDcLinkSetBacklightLevelPattern[]     = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55,
                                                            0x41, 0x54, 0x53, 0x50, 0x41, 0x89, 0xD6, 0x41, 0x89, 0xF4};
static const UInt8 kDcLinkSetBacklightLevelPattern1404[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56,
                                                            0x41, 0x55, 0x41, 0x54, 0x53, 0x50, 0x89, 0xD3,
                                                            0x41, 0x89, 0xF6, 0x49, 0x89, 0xFC};

static const UInt8 kDcLinkSetBacklightLevelNitsPattern[]     = {0x55, 0x48, 0x89, 0xE5, 0x53, 0x50, 0x40, 0x88,
                                                                0x75, 0x00, 0x48, 0x85, 0xFF, 0x74, 0x00};
static const UInt8 kDcLinkSetBacklightLevelNitsPatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                                0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};

static constexpr UInt32 kIOAppleBacklightAttribute = 'bklt';

static Backlight moduleInstance;

Backlight& Backlight::singleton() { return moduleInstance; }

Backlight::Backlight()
{
    if (currentKernelVersion() <= MACOS_10_15_X) { this->dcLinkCapsField = 0x1EA; }
    else if (currentKernelVersion().majorMatches(MACOS_11)) {
        this->dcLinkCapsField = 0x26C;
    }
    else if (currentKernelVersion().majorMatches(MACOS_12)) {
        this->dcLinkCapsField = 0x284;
    }
    else {
        this->dcLinkCapsField = 0x28C;
    }
}

void Backlight::init()
{
    auto bkltArg = BaseDeviceInfo::get().modelType == WIOKit::ComputerModel::ComputerLaptop;
    PE_parse_boot_argn("AMDBacklight", &bkltArg, sizeof(bkltArg));
    if (!bkltArg) { return; }

    lilu.onKextLoadForce(&kextAppleBacklight);
    lilu.onKextLoadForce(&kextAppleMCCSControl);

    auto matching = IOService::serviceMatching("AppleBacklightDisplay");
    if (matching == nullptr) {
        SYSLOG("Backlight", "Failed to create match dictionary");
        return;
    }

    this->dispNotif =
        IOService::addMatchingNotification(gIOFirstMatchNotification, matching, OnAppleBacklightDisplayLoad, nullptr);
    SYSLOG_COND(this->dispNotif == nullptr, "Backlight", "Failed to register notification");
    OSSafeReleaseNULL(matching);
}

void Backlight::processKext(KernelPatcher& patcher, const size_t id, const mach_vm_address_t slide, const size_t size)
{
    if (kextRadeonX6000Framebuffer.loadIndex == id) {
        if (currentKernelVersion() >= MACOS_14_4) {
            PenguinWizardry::PatternSolveRequest solveRequest{
                "_dc_link_set_backlight_level", this->orgDcLinkSetBacklightLevel, kDcLinkSetBacklightLevelPattern1404};
            PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "Backlight",
                       "Failed to resolve dc_link_set_backlight_level");
        }
        else {
            PenguinWizardry::PatternSolveRequest solveRequest{
                "_dc_link_set_backlight_level", this->orgDcLinkSetBacklightLevel, kDcLinkSetBacklightLevelPattern};
            PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "Backlight",
                       "Failed to resolve dc_link_set_backlight_level");
        }

        PenguinWizardry::PatternSolveRequest solveRequest{
            "_dc_link_set_backlight_level_nits", this->orgDcLinkSetBacklightLevelNits,
            kDcLinkSetBacklightLevelNitsPattern, kDcLinkSetBacklightLevelNitsPatternMask};
        PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "Backlight",
                   "Failed to resolve dc_link_set_backlight_level_nits");
        PenguinWizardry::PatternRouteRequest requests[] = {
            {"_link_create", wrapLinkCreate, this->orgLinkCreate, kLinkCreatePattern, kLinkCreatePatternMask},
            {"__ZN35AMDRadeonX6000_AmdRadeonFramebuffer25setAttributeForConnectionEijm", wrapSetAttributeForConnection,
             this->orgSetAttributeForConnection},
            {"__ZN35AMDRadeonX6000_AmdRadeonFramebuffer25getAttributeForConnectionEijPm", wrapGetAttributeForConnection,
             this->orgGetAttributeForConnection},
        };
        PANIC_COND(!PenguinWizardry::PatternRouteRequest::routeAll(patcher, id, requests, slide, size), "Backlight",
                   "Failed to route backlight symbols");
    }
    else if (kextAppleBacklight.loadIndex == id) {
        KernelPatcher::RouteRequest request{"__ZN15AppleIntelPanel10setDisplayEP9IODisplay", wrapApplePanelSetDisplay,
                                            orgApplePanelSetDisplay};
        if (patcher.routeMultiple(kextAppleBacklight.loadIndex, &request, 1, slide, size)) {
            static const UInt8                       find[]    = "F%uT%04x";
            static const UInt8                       replace[] = "F%uTxxxx";
            const PenguinWizardry::MaskedLookupPatch patch{&kextAppleBacklight, find, replace, 1};
            SYSLOG_COND(!patch.apply(patcher, slide, size), "Backlight", "Failed to apply backlight patch");
        }
        patcher.clearError();
    }
    else if (kextAppleMCCSControl.loadIndex == id) {
        KernelPatcher::RouteRequest requests[] = {
            {"__ZN25AppleMCCSControlGibraltar5probeEP9IOServicePi", returnZero},
            {"__ZN21AppleMCCSControlCello5probeEP9IOServicePi", returnZero},
        };
        patcher.routeMultiple(id, requests, slide, size);
        patcher.clearError();
    }
}

bool Backlight::OnAppleBacklightDisplayLoad(void* const, void* const, IOService* const newService, IONotifier* const)
{
    const auto params = OSDynamicCast(OSDictionary, newService->getProperty("IODisplayParameters"));
    if (params == nullptr) { return false; }

    const auto linearBrightness = OSDynamicCast(OSDictionary, params->getObject("linear-brightness"));
    if (linearBrightness == nullptr) { return false; }

    const auto maxBrightness = OSDynamicCast(OSNumber, linearBrightness->getObject("max"));
    if (maxBrightness == nullptr) { return false; }

    if (maxBrightness->unsigned32BitValue() == 0) { return false; }

    singleton().maxBacklightLvl = maxBrightness->unsigned32BitValue();

    return true;
}

IOReturn Backlight::wrapSetAttributeForConnection(IOService* const framebuffer, const IOIndex connectIndex,
                                                  const IOSelect attribute, const uintptr_t value)
{
    const auto ret = FunctionCast(wrapSetAttributeForConnection, singleton().orgSetAttributeForConnection)(
        framebuffer, connectIndex, attribute, value);
    if (attribute != kIOAppleBacklightAttribute) { return ret; }

    singleton().curBacklightLvl = static_cast<UInt32>(value);

    if (singleton().embeddedPanelLink == nullptr) { return kIOReturnNoDevice; }

    if (singleton().maxBacklightLvl == 0) { return kIOReturnInternalError; }

    const auto percentage = singleton().curBacklightLvl * 100 / singleton().maxBacklightLvl;

    if (singleton().supportsAUX) {
        const auto auxValue = (singleton().maxBacklightMillinits * percentage) / 100;
        if (singleton().orgDcLinkSetBacklightLevelNits(singleton().embeddedPanelLink, true, auxValue, 50)) {
            return kIOReturnSuccess;
        }
    }
    else {
        const auto pwmValue = (percentage * 0xFFFF) / 100;
        if (singleton().orgDcLinkSetBacklightLevel(singleton().embeddedPanelLink, pwmValue, 0)) {
            return kIOReturnSuccess;
        }
    }

    return kIOReturnDeviceError;
}

IOReturn Backlight::wrapGetAttributeForConnection(IOService* framebuffer, IOIndex connectIndex, IOSelect attribute,
                                                  uintptr_t* value)
{
    const auto ret = FunctionCast(wrapGetAttributeForConnection, singleton().orgGetAttributeForConnection)(
        framebuffer, connectIndex, attribute, value);
    if (attribute != kIOAppleBacklightAttribute) { return ret; }
    *value = singleton().curBacklightLvl;
    return kIOReturnSuccess;
}

void* Backlight::wrapLinkCreate(void* data)
{
    const auto ret = FunctionCast(wrapLinkCreate, singleton().orgLinkCreate)(data);
    if (ret == nullptr) { return ret; }

    const auto signalType = getMember<UInt32>(ret, 0x38);
    switch (signalType) {
        case SIGNAL_TYPE_LVDS: {
            if (singleton().embeddedPanelLink == nullptr) { singleton().embeddedPanelLink = ret; }
            else {
                SYSLOG("Backlight", "Found multiple embedded panel links. This may be a bug. Please report this.");
            }
        } break;
        case SIGNAL_TYPE_EDP: {
            if (singleton().embeddedPanelLink == nullptr) {
                singleton().embeddedPanelLink = ret;
                const auto& linkCaps          = singleton().dcLinkCapsField(ret);
                // TODO: PWM should be used for SDR mode on LCD HDR panels,
                // but AUX for HDR mode. Linux doesn't support HDR properly,
                // but macOS does, so here's another thing to fix.
                singleton().supportsAUX = linkCaps.oled != 0;
            }
            else {
                SYSLOG("Backlight", "Found multiple embedded panel links. This may be a bug. Please report this.");
            }
        } break;
        default: break;
    }

    return ret;
}

struct ApplePanelData
{
    const char* deviceName;
    UInt8       deviceData[36];
};

static ApplePanelData appleBacklightData[] = {
    {"F14Txxxx",
     {0x00, 0x11, 0x00, 0x00, 0x00, 0x34, 0x00, 0x52, 0x00, 0x73, 0x00, 0x94, 0x00, 0xBE, 0x00, 0xFA, 0x01, 0x36,
      0x01, 0x72, 0x01, 0xC5, 0x02, 0x2F, 0x02, 0xB9, 0x03, 0x60, 0x04, 0x1A, 0x05, 0x0A, 0x06, 0x0E, 0x07, 0x10}},
    {"F15Txxxx",
     {0x00, 0x11, 0x00, 0x00, 0x00, 0x36, 0x00, 0x54, 0x00, 0x7D, 0x00, 0xB2, 0x00, 0xF5, 0x01, 0x49, 0x01, 0xB1,
      0x02, 0x2B, 0x02, 0xB8, 0x03, 0x59, 0x04, 0x13, 0x04, 0xEC, 0x05, 0xF3, 0x07, 0x34, 0x08, 0xAF, 0x0A, 0xD9}},
    {"F16Txxxx",
     {0x00, 0x11, 0x00, 0x00, 0x00, 0x18, 0x00, 0x27, 0x00, 0x3A, 0x00, 0x52, 0x00, 0x71, 0x00, 0x96, 0x00, 0xC4,
      0x00, 0xFC, 0x01, 0x40, 0x01, 0x93, 0x01, 0xF6, 0x02, 0x6E, 0x02, 0xFE, 0x03, 0xAA, 0x04, 0x78, 0x05, 0x6C}},
    {"F17Txxxx",
     {0x00, 0x11, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x34, 0x00, 0x4F, 0x00, 0x71, 0x00, 0x9B, 0x00, 0xCF, 0x01, 0x0E,
      0x01, 0x5D, 0x01, 0xBB, 0x02, 0x2F, 0x02, 0xB9, 0x03, 0x60, 0x04, 0x29, 0x05, 0x1E, 0x06, 0x44, 0x07, 0xA1}},
    {"F18Txxxx",
     {0x00, 0x11, 0x00, 0x00, 0x00, 0x53, 0x00, 0x8C, 0x00, 0xD5, 0x01, 0x31, 0x01, 0xA2, 0x02, 0x2E, 0x02, 0xD8,
      0x03, 0xAE, 0x04, 0xAC, 0x05, 0xE5, 0x07, 0x59, 0x09, 0x1C, 0x0B, 0x3B, 0x0D, 0xD0, 0x10, 0xEA, 0x14, 0x99}},
    {"F19Txxxx",
     {0x00, 0x11, 0x00, 0x00, 0x02, 0x8F, 0x03, 0x53, 0x04, 0x5A, 0x05, 0xA1, 0x07, 0xAE, 0x0A, 0x3D, 0x0E, 0x14,
      0x13, 0x74, 0x1A, 0x5E, 0x24, 0x18, 0x31, 0xA9, 0x44, 0x59, 0x5E, 0x76, 0x83, 0x11, 0xB6, 0xC7, 0xFF, 0x7B}},
    {"F24Txxxx",
     {0x00, 0x11, 0x00, 0x01, 0x00, 0x34, 0x00, 0x52, 0x00, 0x73, 0x00, 0x94, 0x00, 0xBE, 0x00, 0xFA, 0x01, 0x36,
      0x01, 0x72, 0x01, 0xC5, 0x02, 0x2F, 0x02, 0xB9, 0x03, 0x60, 0x04, 0x1A, 0x05, 0x0A, 0x06, 0x0E, 0x07, 0x10}},
};

size_t Backlight::returnZero() { return 0; }

bool Backlight::wrapApplePanelSetDisplay(IOService* self, IODisplay* display)
{
    static bool once = false;
    if (!once) {
        once        = true;
        auto panels = OSDynamicCast(OSDictionary, self->getProperty("ApplePanels"));
        if (panels) {
            auto rawPanels = panels->copyCollection();
            panels         = OSDynamicCast(OSDictionary, rawPanels);

            if (panels) {
                for (auto& entry : appleBacklightData) {
                    auto pd = OSData::withBytes(entry.deviceData, sizeof(entry.deviceData));
                    if (pd) {
                        panels->setObject(entry.deviceName, pd);
                        // No release required by current AppleBacklight implementation.
                    }
                    else {
                        SYSLOG("Backlight", "setDisplay: Cannot allocate data for %s", entry.deviceName);
                    }
                }
                self->setProperty("ApplePanels", panels);
            }

            OSSafeReleaseNULL(rawPanels);
        }
        else {
            SYSLOG("Backlight", "setDisplay: Missing ApplePanels property");
        }
    }

    return FunctionCast(wrapApplePanelSetDisplay, singleton().orgApplePanelSetDisplay)(self, display);
}
