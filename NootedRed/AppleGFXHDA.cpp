// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include "AppleGFXHDA.hpp"
#include "NRed.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_api.hpp>

constexpr UInt32 AMDVendorID = 0x1002;
constexpr UInt32 Navi10HDMIDeviceID = 0xAB38;
constexpr UInt32 Navi10HDMIID = (Navi10HDMIDeviceID << 16) | AMDVendorID;
constexpr UInt32 RavenHDMIDeviceID = 0x15DE;
constexpr UInt32 RavenHDMIID = (RavenHDMIDeviceID << 16) | AMDVendorID;
constexpr UInt32 RenoirHDMIDeviceID = 0x1637;
constexpr UInt32 RenoirHDMIID = (RenoirHDMIDeviceID << 16) | AMDVendorID;

static const char *pathAppleGFXHDA = "/System/Library/Extensions/AppleGFXHDA.kext/Contents/MacOS/AppleGFXHDA";

static KernelPatcher::KextInfo kextAppleGFXHDA {
    "com.apple.driver.AppleGFXHDA",
    &pathAppleGFXHDA,
    1,
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

AppleGFXHDA *AppleGFXHDA::callback = nullptr;

void AppleGFXHDA::init() {
    callback = this;
    lilu.onKextLoadForce(&kextAppleGFXHDA);
}

bool AppleGFXHDA::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextAppleGFXHDA.loadIndex == id) {
        NRed::callback->ensureRMMIO();

        const UInt32 probeFind = Navi10HDMIID;
        const UInt32 probeRepl = NRed::callback->attributes.isRenoir() ? RenoirHDMIID : RavenHDMIID;
        const LookupPatchPlus patch = {&kextAppleGFXHDA, reinterpret_cast<const UInt8 *>(&probeFind),
            reinterpret_cast<const UInt8 *>(&probeRepl), sizeof(probeFind), 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "AGFXHDA", "Failed to apply patch for HDMI controller probe");

        SolveRequestPlus solveRequests[] = {
            {"__ZN34AppleGFXHDAFunctionGroupATI_Tahiti10gMetaClassE", this->orgFunctionGroupTahiti},
            {"__ZN26AppleGFXHDAWidget_1002AAA010gMetaClassE", this->orgWidget1002AAA0},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, id, solveRequests, slide, size), "AGFXHDA",
            "Failed to solve symbols");

        RouteRequestPlus requests[] = {
            {"__ZN31AppleGFXHDAFunctionGroupFactory27createAppleHDAFunctionGroupEP11DevIdStruct",
                wrapCreateAppleHDAFunctionGroup, this->orgCreateAppleHDAFunctionGroup},
            {"__ZN24AppleGFXHDAWidgetFactory20createAppleHDAWidgetEP11DevIdStruct", wrapCreateAppleHDAWidget,
                this->orgCreateAppleHDAWidget},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "AGFXHDA",
            "Failed to route symbols");

        return true;
    }

    return false;
}

void *AppleGFXHDA::wrapCreateAppleHDAFunctionGroup(void *devId) {
    auto vendorID = getMember<UInt16>(devId, 0x2);
    auto deviceID = getMember<UInt32>(devId, 0x8);
    if (vendorID == AMDVendorID && (deviceID == RavenHDMIDeviceID || deviceID == RenoirHDMIDeviceID)) {
        return callback->orgFunctionGroupTahiti->alloc();
    }
    return FunctionCast(wrapCreateAppleHDAFunctionGroup, callback->orgCreateAppleHDAFunctionGroup)(devId);
}

void *AppleGFXHDA::wrapCreateAppleHDAWidget(void *devId) {
    auto vendorID = getMember<UInt16>(devId, 0x2);
    auto deviceID = getMember<UInt32>(devId, 0x8);
    if (vendorID == AMDVendorID && (deviceID == RavenHDMIDeviceID || deviceID == RenoirHDMIDeviceID)) {
        return callback->orgWidget1002AAA0->alloc();
    }
    return FunctionCast(wrapCreateAppleHDAFunctionGroup, callback->orgCreateAppleHDAFunctionGroup)(devId);
}
