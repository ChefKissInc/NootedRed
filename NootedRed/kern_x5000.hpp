//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include "kern_amd.hpp"
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>

class X5000 {
    friend class X6000;

    public:
    static X5000 *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    t_HWEngineConstructor orgGFX9PM4EngineConstructor {nullptr};
    t_HWEngineConstructor orgGFX9SDMAEngineConstructor {nullptr};
    mach_vm_address_t orgSetupAndInitializeHWCapabilities {0};
    mach_vm_address_t orgRTGetHWChannel {0};
    mach_vm_address_t orgAdjustVRAMAddress {0};
    mach_vm_address_t orgAccelSharedUCStart {0};
    mach_vm_address_t orgAccelSharedUCStop {0};
    mach_vm_address_t orgAllocateAMDHWAlignManager {0};
    mach_vm_address_t orgSetupAndInitializeHWCapabilitiesVega20 {0};

    static bool wrapAllocateHWEngines(void *that);
    static void wrapSetupAndInitializeHWCapabilities(void *that);
    static void *wrapRTGetHWChannel(void *that, uint32_t param1, uint32_t param2, uint32_t param3);
    static void wrapInitializeFamilyType(void *that);
    static void *wrapAllocateAMDHWDisplay(void *that);
    static uint64_t wrapAdjustVRAMAddress(void *that, uint64_t addr);
    static void *wrapNewVideoContext(void *that);
    static void *wrapCreateSMLInterface(uint32_t configBit);
    static void *wrapNewShared();
    static void *wrapNewSharedUserClient();
    static void *wrapAllocateAMDHWAlignManager();
};