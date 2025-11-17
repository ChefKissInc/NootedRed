// AMDRadeonX6000Framebuffer hotfixes
//
// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_api.hpp>
#include <Hotfixes/X6000FB.hpp>
#include <Kexts.hpp>
#include <NRed.hpp>
#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/PatcherPlus.hpp>

static const UInt8 kDpReceiverPowerCtrlPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54, 0x53,
    0x48, 0x83, 0xEC, 0x10, 0x89, 0xF3, 0xB0, 0x02, 0x28, 0xD8};
static const UInt8 kDpReceiverPowerCtrlPattern1404[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54,
    0x53, 0x48, 0x83, 0xEC, 0x10, 0x41, 0x89, 0xF7, 0xB0, 0x02, 0x44, 0x28, 0xF8};

// Remove new FB count condition so we can restore the original behaviour before Ventura.
static const UInt8 kControllerPowerUpOriginal[] = {0x38, 0xC8, 0x0F, 0x42, 0xC8, 0x88, 0x8F, 0xBC, 0x00, 0x00, 0x00,
    0x72, 0x00};
static const UInt8 kControllerPowerUpOriginalMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00};
static const UInt8 kControllerPowerUpReplace[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xEB, 0x00};
static const UInt8 kControllerPowerUpReplaceMask[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0x00};

// Remove new problematic Ventura pixel clock multiplier calculation which causes timing validation mishaps.
static const UInt8 kValidateDetailedTimingOriginal[] = {0x66, 0x0F, 0x2E, 0xC1, 0x76, 0x06, 0xF2, 0x0F, 0x5E, 0xC1};
static const UInt8 kValidateDetailedTimingPatched[] = {0x66, 0x0F, 0x2E, 0xC1, 0x66, 0x90, 0xF2, 0x0F, 0x5E, 0xC1};

static Hotfixes::X6000FB instance {};

Hotfixes::X6000FB &Hotfixes::X6000FB::singleton() { return instance; }

void Hotfixes::X6000FB::processKext(KernelPatcher &patcher, const size_t id, const mach_vm_address_t slide,
    const size_t size) {
    if (kextRadeonX6000Framebuffer.loadIndex != id) { return; }

    if (checkKernelArgument("-NRedDPDelay")) {
        if (currentKernelVersion() >= MACOS_14_4) {
            PenguinWizardry::PatternRouteRequest request {"_dp_receiver_power_ctrl", wrapDpReceiverPowerCtrl,
                this->orgDpReceiverPowerCtrl, kDpReceiverPowerCtrlPattern1404};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB",
                "Failed to route dp_receiver_power_ctrl (14.4+)");
        } else {
            PenguinWizardry::PatternRouteRequest request {"_dp_receiver_power_ctrl", wrapDpReceiverPowerCtrl,
                this->orgDpReceiverPowerCtrl, kDpReceiverPowerCtrlPattern};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB", "Failed to route dp_receiver_power_ctrl");
        }
    }

    if (currentKernelVersion() >= MACOS_13) {
        this->orgMessageAccelerator = patcher.solveSymbol<decltype(this->orgMessageAccelerator)>(id,
            "__ZNK34AMDRadeonX6000_AmdRadeonController18messageAcceleratorE25_eAMDAccelIOFBRequestTypePvS1_S1_", slide,
            size);
        PANIC_COND(this->orgMessageAccelerator == nullptr, "X6000FB", "Failed to resolve messageAccelerator");

        KernelPatcher::RouteRequest request {"__ZN34AMDRadeonX6000_AmdRadeonController7powerUpEv",
            wrapControllerPowerUp, this->orgControllerPowerUp};
        PANIC_COND(!patcher.routeMultiple(id, &request, 1, slide, size), "X6000FB", "Failed to route powerUp");

        const PenguinWizardry::MaskedLookupPatch patches[] = {
            {&kextRadeonX6000Framebuffer, kControllerPowerUpOriginal, kControllerPowerUpOriginalMask,
                kControllerPowerUpReplace, kControllerPowerUpReplaceMask, 1},
            {&kextRadeonX6000Framebuffer, kValidateDetailedTimingOriginal, kValidateDetailedTimingPatched, 1},
        };
        PANIC_COND(!PenguinWizardry::MaskedLookupPatch::applyAll(patcher, patches, slide, size), "X6000FB",
            "Failed to apply logic revert patches");
    }
}

UInt32 Hotfixes::X6000FB::wrapControllerPowerUp(void *self) {
    auto &m_flags = getMember<UInt8>(self, 0x5F18);
    auto send = (m_flags & 2) == 0;
    m_flags |= 4;    // All framebuffers enabled
    auto ret = FunctionCast(wrapControllerPowerUp, singleton().orgControllerPowerUp)(self);
    if (send) { singleton().orgMessageAccelerator(self, IOFBRequestControllerEnabled, nullptr, nullptr, nullptr); }
    return ret;
}

void Hotfixes::X6000FB::wrapDpReceiverPowerCtrl(void *link, bool power_on) {
    FunctionCast(wrapDpReceiverPowerCtrl, singleton().orgDpReceiverPowerCtrl)(link, power_on);
    IOSleep(250);
}
