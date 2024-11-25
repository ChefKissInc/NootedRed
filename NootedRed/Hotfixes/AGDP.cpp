// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_api.hpp>
#include <PrivateHeaders/Hotfixes/AGDP.hpp>
#include <PrivateHeaders/NRed.hpp>
#include <PrivateHeaders/PatcherPlus.hpp>

//------ Target Kexts ------//

static const char *pathAGDP = "/System/Library/Extensions/AppleGraphicsControl.kext/Contents/PlugIns/"
                              "AppleGraphicsDevicePolicy.kext/Contents/MacOS/AppleGraphicsDevicePolicy";
static KernelPatcher::KextInfo kextAGDP {
    "com.apple.driver.AppleGraphicsDevicePolicy",
    &pathAGDP,
    1,
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

//------ Patches ------//

// Change frame-buffer count >= 2 check to >= 1.
static const UInt8 kAGDPFBCountCheckOriginal[] = {0x02, 0x00, 0x00, 0x83, 0xF8, 0x02};
static const UInt8 kAGDPFBCountCheckPatched[] = {0x02, 0x00, 0x00, 0x83, 0xF8, 0x01};

// Ditto
static const UInt8 kAGDPFBCountCheckOriginal13[] = {0x41, 0x83, 0xBE, 0x14, 0x02, 0x00, 0x00, 0x02};
static const UInt8 kAGDPFBCountCheckPatched13[] = {0x41, 0x83, 0xBE, 0x14, 0x02, 0x00, 0x00, 0x01};

// Neutralise access to AGDP configuration by board identifier.
static const UInt8 kAGDPBoardIDKeyOriginal[] = "board-id";
static const UInt8 kAGDPBoardIDKeyPatched[] = "applehax";

//------ Module Logic ------//

static Hotfixes::AGDP instance {};

Hotfixes::AGDP &Hotfixes::AGDP::singleton() { return instance; }

void Hotfixes::AGDP::init() {
    PANIC_COND(this->initialised, "AGDP", "Attempted to initialise module twice!");
    this->initialised = true;

    SYSLOG("AGDP", "Module initialised.");

    lilu.onKextLoadForce(
        &kextAGDP, 1,
        [](void *user, KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
            static_cast<Hotfixes::AGDP *>(user)->processKext(patcher, id, slide, size);
        },
        this);
}

void Hotfixes::AGDP::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextAGDP.loadIndex != id) { return; }

    const LookupPatchPlus boardIdPatch {&kextAGDP, kAGDPBoardIDKeyOriginal, kAGDPBoardIDKeyPatched, 1};
    SYSLOG_COND(!boardIdPatch.apply(patcher, slide, size), "AGDP", "Failed to apply AGDP board-id patch");

    if (NRed::singleton().getAttributes().isVentura()) {
        const LookupPatchPlus patch {&kextAGDP, kAGDPFBCountCheckOriginal13, kAGDPFBCountCheckPatched13, 1};
        SYSLOG_COND(!patch.apply(patcher, slide, size), "AGDP", "Failed to apply AGDP FB count check patch");
    } else {
        const LookupPatchPlus patch {&kextAGDP, kAGDPFBCountCheckOriginal, kAGDPFBCountCheckPatched, 1};
        SYSLOG_COND(!patch.apply(patcher, slide, size), "AGDP", "Failed to apply AGDP FB count check patch");
    }
}
