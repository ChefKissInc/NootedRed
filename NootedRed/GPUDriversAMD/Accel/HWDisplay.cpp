// Version-independent interface to the `AMDRadeonX5000_AMDHWDisplay` class
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <GPUDriversAMD/Accel/HWDisplay.hpp>
#include <PenguinWizardry/KernelVersion.hpp>

AMDHWDisplayState::Status::Constants AMDHWDisplayState::Status::constants {};

AMDRadeonX5000_AMDHWDisplay::Constants AMDRadeonX5000_AMDHWDisplay::constants {};

void AMDRadeonX5000_AMDHWDisplay::resolve(KernelPatcher &patcher, const size_t id, const mach_vm_address_t slide,
    const size_t size) {
    constants.vfuncs =
        patcher.solveSymbol<decltype(constants.vfuncs)>(id, "__ZTV27AMDRadeonX5000_AMDHWDisplay", slide, size, true);
    PANIC_COND(constants.vfuncs == nullptr, "HWDisplay", "Failed to resolve VTable");
    constants.vfuncs += 2;

    constants.init = patcher.solveSymbol(id,
        "__ZN27AMDRadeonX5000_AMDHWDisplay4initEP30AMDRadeonX5000_IAMDHWInterfaceP14_FB_PARAMETERS", slide, size, true);
    PANIC_COND(constants.init == 0, "HWDisplay", "Failed to solve init");
    constants.constructor =
        patcher.solveSymbol(id, "__ZN27AMDRadeonX5000_AMDHWDisplayC2EPK11OSMetaClass", slide, size, true);
    PANIC_COND(constants.constructor == 0, "HWDisplay", "Failed to solve constructor");
}
