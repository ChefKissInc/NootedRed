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
        const uint32_t probeFind = 0xAB381002;
        const uint32_t probeRepl = NRed::callback->deviceId <= 0x15DD ? 0x15DE1002 : 0x16371002;
        LookupPatchPlus patches[] = {
            {&kextAppleGFXHDA, reinterpret_cast<const uint8_t *>(&probeFind),
                reinterpret_cast<const uint8_t *>(&probeRepl), sizeof(probeFind), 1},
            {&kextAppleGFXHDA, kCreateAppleHDAFunctionGroup1Original, kCreateAppleHDAFunctionGroup1Mask,
                kCreateAppleHDAFunctionGroup1Replace, 1},
            {&kextAppleGFXHDA, kCreateAppleHDAFunctionGroup2Original, kCreateAppleHDAFunctionGroup2Mask,
                kCreateAppleHDAFunctionGroup2Replace, 1},
            {&kextAppleGFXHDA, kCreateAppleHDAWidget1Original, kCreateAppleHDAWidget1Mask,
                kCreateAppleHDAWidget1Replace, 1},
            {&kextAppleGFXHDA, kCreateAppleHDAWidget2Original, kCreateAppleHDAWidget2Mask,
                kCreateAppleHDAWidget2Replace, 1},
            {&kextAppleGFXHDA, kCreateAppleHDAOriginal, kCreateAppleHDAMask, kCreateAppleHDAReplace, 2},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(&patcher, patches, address, size), "agfxhda",
            "Failed to apply patches: %d", patcher.getError());

        return true;
    }

    return false;
}
