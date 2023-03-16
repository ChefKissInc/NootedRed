//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#ifndef kern_x6000_hpp
#define kern_x6000_hpp
#include "kern_amd.hpp"
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>

class X6000 {
    friend class X5000;

    public:
    static X6000 *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    t_HWEngineConstructor orgVCN2EngineConstructor {nullptr};
    mach_vm_address_t orgAllocateAMDHWDisplay {0};
    mach_vm_address_t orgNewVideoContext {0};
    mach_vm_address_t orgCreateSMLInterface {0};
    mach_vm_address_t orgNewShared {0};
    mach_vm_address_t orgNewSharedUserClient {0};
    mach_vm_address_t orgInitDCNRegistersOffsets {0};
    mach_vm_address_t orgGetPreferredSwizzleMode2 {0};
    mach_vm_address_t orgAccelSharedSurfaceCopy {0};
    mach_vm_address_t orgAllocateScanoutFB {0};
    mach_vm_address_t orgFillUBMSurface {0};
    mach_vm_address_t orgConfigureDisplay {0};
    mach_vm_address_t orgGetDisplayInfo {0};

    static bool wrapAccelStartX6000();
    static bool wrapAccelSharedUCStartX6000(void *that, void *provider);
    static bool wrapAccelSharedUCStopX6000(void *that, void *provider);
    static void wrapInitDCNRegistersOffsets(void *that);
    static uint64_t wrapAccelSharedSurfaceCopy(void *that, void *param1, uint64_t param2, void *param3);
    static uint64_t wrapAllocateScanoutFB(void *that, uint32_t param1, void *param2, void *param3, void *param4);
    static uint64_t wrapFillUBMSurface(void *that, uint32_t param1, void *param2, void *param3);
    static bool wrapConfigureDisplay(void *that, uint32_t param1, uint32_t param2, void *param3, void *param4);
    static uint64_t wrapGetDisplayInfo(void *that, uint32_t param1, bool param2, bool param3, void *param4,
        void *param5);
};

#endif /* kern_x6000_hpp */
