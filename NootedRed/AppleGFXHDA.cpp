//! Copyright © 2022-2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "AppleGFXHDA.hpp"
#include "NRed.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_api.hpp>

static const char *pathAppleGFXHDA = "/System/Library/Extensions/AppleGFXHDA.kext/Contents/MacOS/AppleGFXHDA";

static KernelPatcher::KextInfo kextAppleGFXHDA {"com.apple.driver.AppleGFXHDA", &pathAppleGFXHDA, 1, {true}, {},
    KernelPatcher::KextInfo::Unloaded};

AppleGFXHDA *AppleGFXHDA::callback = nullptr;

void AppleGFXHDA::init() {
    callback = this;
    lilu.onKextLoadForce(&kextAppleGFXHDA);
}

bool AppleGFXHDA::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextAppleGFXHDA.loadIndex == id) {
        const UInt32 probeFind = 0xAB381002;
        const UInt32 probeRepl = NRed::callback->deviceId <= 0x15DD ? 0x15DE1002 : 0x16371002;
        bool catalina = getKernelVersion() == KernelVersion::Catalina;
        const LookupPatchPlus patches[] = {
            {&kextAppleGFXHDA, reinterpret_cast<const UInt8 *>(&probeFind), reinterpret_cast<const UInt8 *>(&probeRepl),
                sizeof(probeFind), 1},
            {&kextAppleGFXHDA, kCreateAppleHDAWidget1Original, kCreateAppleHDAWidget1OriginalMask,
                kCreateAppleHDAWidget1Patched, kCreateAppleHDAWidget1PatchedMask, catalina ? 2U : 1},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "AGFXHDA", "Failed to apply patches");

        if (catalina) {
            const LookupPatchPlus patch {&kextAppleGFXHDA, kCreateAppleHDACatalinaOriginal,
                kCreateAppleHDACatalinaOriginalMask, kCreateAppleHDACatalinaPatched, kCreateAppleHDACatalinaPatchedMask,
                1};
            PANIC_COND(!patch.apply(patcher, slide, size), "AGFXHDA", "Failed to apply patch");
        } else {
            const LookupPatchPlus patches[] = {
                {&kextAppleGFXHDA, kCreateAppleHDAFunctionGroup1Original, kCreateAppleHDAFunctionGroup1Patched, 1},
                {&kextAppleGFXHDA, kCreateAppleHDAFunctionGroup2Original, kCreateAppleHDAFunctionGroup2Patched, 1},
                {&kextAppleGFXHDA, kCreateAppleHDAWidget2Original, kCreateAppleHDAWidget2OriginalMask,
                    kCreateAppleHDAWidget2Patched, 1},
                {&kextAppleGFXHDA, kCreateAppleHDAOriginal, kCreateAppleHDAOriginalMask, kCreateAppleHDAPatched,
                    kCreateAppleHDAPatchedMask, 2},
                {&kextAppleGFXHDA, kCreateAppleHDA2Original, kCreateAppleHDA2OriginalMask, kCreateAppleHDA2Patched,
                    kCreateAppleHDA2PatchedMask, 2},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "AGFXHDA", "Failed to apply patches");
        }

        return true;
    }

    return false;
}
