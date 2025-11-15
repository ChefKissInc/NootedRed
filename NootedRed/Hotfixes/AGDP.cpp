// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_api.hpp>
#include <Hotfixes/AGDP.hpp>
#include <Kexts.hpp>
#include <NRed.hpp>
#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/PatcherPlus.hpp>

// Change frame-buffer count >= 2 check to >= 1.
static const UInt8 kAGDPFBCountCheckOriginal[] = {0x02, 0x00, 0x00, 0x83, 0xF8, 0x02};
static const UInt8 kAGDPFBCountCheckPatched[] = {0x02, 0x00, 0x00, 0x83, 0xF8, 0x01};

// Ditto
static const UInt8 kAGDPFBCountCheckOriginal13[] = {0x41, 0x83, 0xBE, 0x14, 0x02, 0x00, 0x00, 0x02};
static const UInt8 kAGDPFBCountCheckPatched13[] = {0x41, 0x83, 0xBE, 0x14, 0x02, 0x00, 0x00, 0x01};

// Neutralise access to AGDP configuration by board identifier.
static const UInt8 kAGDPBoardIDKeyOriginal[] = "board-id";
static const UInt8 kAGDPBoardIDKeyPatched[] = "applehax";

static Hotfixes::AGDP instance {};

Hotfixes::AGDP &Hotfixes::AGDP::singleton() { return instance; }

void Hotfixes::AGDP::processKext(KernelPatcher &patcher, const size_t id, const mach_vm_address_t slide,
    const size_t size) {
    if (kextAGDP.loadIndex != id) { return; }

    const PatcherPlus::MaskedLookupPatch boardIdPatch {&kextAGDP, kAGDPBoardIDKeyOriginal, kAGDPBoardIDKeyPatched, 1};
    SYSLOG_COND(!boardIdPatch.apply(patcher, slide, size), "AGDP", "Failed to apply AGDP board-id patch");

    if (currentKernelVersion() == MACOS_13) {
        const PatcherPlus::MaskedLookupPatch patch {&kextAGDP, kAGDPFBCountCheckOriginal13, kAGDPFBCountCheckPatched13,
            1};
        SYSLOG_COND(!patch.apply(patcher, slide, size), "AGDP", "Failed to apply AGDP FB count check patch");
    } else {
        const PatcherPlus::MaskedLookupPatch patch {&kextAGDP, kAGDPFBCountCheckOriginal, kAGDPFBCountCheckPatched, 1};
        SYSLOG_COND(!patch.apply(patcher, slide, size), "AGDP", "Failed to apply AGDP FB count check patch");
    }
}
