//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_agfxhda.hpp"
#include "kern_nred.hpp"
#include "kern_patcherplus.hpp"
#include "kern_patches.hpp"
#include "kern_patterns.hpp"
#include <Headers/kern_api.hpp>

static const char *pathAppleGFXHDA = "/System/Library/Extensions/AppleGFXHDA.kext/Contents/MacOS/AppleGFXHDA";

static KernelPatcher::KextInfo kextAppleGFXHDA {"com.apple.driver.AppleGFXHDA", &pathAppleGFXHDA, 1, {true}, {},
    KernelPatcher::KextInfo::Unloaded};

AppleGFXHDA *AppleGFXHDA::callback = nullptr;

void AppleGFXHDA::init() {
    callback = this;
    lilu.onKextLoadForce(&kextAppleGFXHDA);
}

bool AppleGFXHDA::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextAppleGFXHDA.loadIndex == index) {
        NRed::callback->setRMMIOIfNecessary();

        /*
                SolveRequestPlus solveRequests[] = {
                    {"__ZL15deviceTypeTable", orgDeviceTypeTable, kDeviceTypeTablePattern},
                };
                PANIC_COND(!SolveRequestPlus::solveAll(&patcher, index, solveRequests, address, size), "agfxhda",
                    "Failed to resolve symbols");
        */

        RouteRequestPlus requests[] = {
            {"__ZN17AppleGFXHDADriver20logControllerCommandEjPjib", wrapLogControllerCommand},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "agfxhda",
            "Failed to route symbols");

        /*
                LookupPatchPlus const patches[] = {
                    {&kextRadeonX5000HWLibs, kPspSwInitOriginal1, kPspSwInitPatched1, 1},
                };
                PANIC_COND(!LookupPatchPlus::applyAll(&patcher, patches, address, size), "agfxhda",
                    "Failed to apply patches: %d", patcher.getError());
        */

        uint32_t const probeFind = 0xAB381002;
        uint32_t const probeRepl = NRed::callback->deviceId <= 0x15DD ? 0x15D71002 : 0x16371002;
        LookupPatchPlus patch {&kextAppleGFXHDA, reinterpret_cast<uint8_t const *>(&probeFind),
            reinterpret_cast<uint8_t const *>(&probeRepl), sizeof(probeFind), 1};
        PANIC_COND(!patch.apply(&patcher, address, size), "nred",
            "Failed to apply AppleGFXHDAController::probe patch: %d", patcher.getError());

        return true;
    }

    return false;
}

void AppleGFXHDA::wrapLogControllerCommand(void *that, uint32_t cmd, uint32_t *output, uint32_t controllerRet,
    bool isAfterExecution) {
    DBGLOG("agfxhda",
        "logControllerCommand << (that: %p cmd: 0x%08X output: 0x%X controllerRet: 0x%X isAfterExecution: %d)", that,
        cmd, output == nullptr ? 0 : *output, controllerRet, isAfterExecution);
}
