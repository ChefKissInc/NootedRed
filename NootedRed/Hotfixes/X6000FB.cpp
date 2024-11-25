// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_api.hpp>
#include <PrivateHeaders/Hotfixes/X6000FB.hpp>
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

//------ Patterns ------//

static const UInt8 kDpReceiverPowerCtrlPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54, 0x53,
    0x48, 0x83, 0xEC, 0x10, 0x89, 0xF3, 0xB0, 0x02, 0x28, 0xD8};
static const UInt8 kDpReceiverPowerCtrlPattern1404[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54,
    0x53, 0x48, 0x83, 0xEC, 0x10, 0x41, 0x89, 0xF7, 0xB0, 0x02, 0x44, 0x28, 0xF8};

//------ Patches ------//

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

//------ Module Logic ------//

static Hotfixes::X6000FB instance {};

Hotfixes::X6000FB &Hotfixes::X6000FB::singleton() { return instance; }

void Hotfixes::X6000FB::init() {
    PANIC_COND(this->initialised, "X6000FB", "Attempted to initialise module twice!");
    this->initialised = true;

    SYSLOG("X6000FB", "Module initialised.");

    lilu.onKextLoadForce(
        &kextRadeonX6000Framebuffer, 1,
        [](void *user, KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
            static_cast<Hotfixes::X6000FB *>(user)->processKext(patcher, id, slide, size);
        },
        this);
}

void Hotfixes::X6000FB::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX6000Framebuffer.loadIndex != id) { return; }

    if (checkKernelArgument("-NRedDPDelay")) {
        if (NRed::singleton().getAttributes().isSonoma1404AndLater()) {
            RouteRequestPlus request {"_dp_receiver_power_ctrl", wrapDpReceiverPowerCtrl, this->orgDpReceiverPowerCtrl,
                kDpReceiverPowerCtrlPattern1404};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB",
                "Failed to route dp_receiver_power_ctrl (14.4+)");
        } else {
            RouteRequestPlus request {"_dp_receiver_power_ctrl", wrapDpReceiverPowerCtrl, this->orgDpReceiverPowerCtrl,
                kDpReceiverPowerCtrlPattern};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB", "Failed to route dp_receiver_power_ctrl");
        }
    }

    if (NRed::singleton().getAttributes().isVenturaAndLater()) {
        SolveRequestPlus solveRequest {
            "__ZNK34AMDRadeonX6000_AmdRadeonController18messageAcceleratorE25_eAMDAccelIOFBRequestTypePvS1_S1_",
            this->orgMessageAccelerator};
        PANIC_COND(!solveRequest.solve(patcher, id, slide, size), "X6000FB", "Failed to resolve messageAccelerator");
    }

    if (NRed::singleton().getAttributes().isVenturaAndLater()) {
        RouteRequestPlus request {"__ZN34AMDRadeonX6000_AmdRadeonController7powerUpEv", wrapControllerPowerUp,
            this->orgControllerPowerUp};
        PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB", "Failed to route powerUp");
    }

    if (NRed::singleton().getAttributes().isVenturaAndLater()) {
        const LookupPatchPlus patches[] = {
            {&kextRadeonX6000Framebuffer, kControllerPowerUpOriginal, kControllerPowerUpOriginalMask,
                kControllerPowerUpReplace, kControllerPowerUpReplaceMask, 1},
            {&kextRadeonX6000Framebuffer, kValidateDetailedTimingOriginal, kValidateDetailedTimingPatched, 1},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "X6000FB",
            "Failed to apply logic revert patches");
    }
}

UInt32 Hotfixes::X6000FB::wrapControllerPowerUp(void *that) {
    auto &m_flags = getMember<UInt8>(that, 0x5F18);
    auto send = (m_flags & 2) == 0;
    m_flags |= 4;    // All framebuffers enabled
    auto ret = FunctionCast(wrapControllerPowerUp, singleton().orgControllerPowerUp)(that);
    if (send) { singleton().orgMessageAccelerator(that, IOFBRequestControllerEnabled, nullptr, nullptr, nullptr); }
    return ret;
}

void Hotfixes::X6000FB::wrapDpReceiverPowerCtrl(void *link, bool power_on) {
    FunctionCast(wrapDpReceiverPowerCtrl, singleton().orgDpReceiverPowerCtrl)(link, power_on);
    IOSleep(250);
}
