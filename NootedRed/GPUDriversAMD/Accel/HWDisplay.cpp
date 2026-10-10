// Version-independent interface to the `AMDRadeonX5000_AMDHWDisplay` class
//
// Copyright (C) 2025-2026  ChefKiss.
//
// Licensed under the ChefKiss Open Knowledge License version 1.
//
// This work is licensed to you under the terms of the ChefKiss Open Knowledge
// License, either version 1 of the License, or (at your option) any later
// version published by ChefKiss.
//
// This work may not be used to train or develop artificial intelligence,
// and may not be used with artificial intelligence in any other way. See
// the ChefKiss Open Knowledge License for details.
//
// This work is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// ChefKiss Open Knowledge License for more details.
//
// You should have received a copy of the ChefKiss Open Knowledge License
// along with this work.  If not, see https://git.chefkiss.dev/ChefKiss/OpenKnowledge/raw/branch/master/LICENSE-v1.md.

#include <GPUDriversAMD/Accel/HWDisplay.hpp>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>
#include <mach/i386/vm_types.h>

AMDHWDisplayState::Status::Constants AMDHWDisplayState::Status::constants;

AMDRadeonX5000_AMDHWDisplay::Constants AMDRadeonX5000_AMDHWDisplay::constants;

void AMDRadeonX5000_AMDHWDisplay::resolve(KernelPatcher& patcher, const size_t id, const mach_vm_address_t slide,
                                          const size_t size)
{
    constants.vft.resolve(patcher, id, "__ZTV27AMDRadeonX5000_AMDHWDisplay", slide, size);

    constants.init = patcher.solveSymbol<decltype(constants.init)>(
        id, "__ZN27AMDRadeonX5000_AMDHWDisplay4initEP30AMDRadeonX5000_IAMDHWInterfaceP14_FB_PARAMETERS", slide, size,
        true);
    PANIC_COND(constants.init == nullptr, "HWDisplay", "Failed to solve init");
    constants.constructor =
        patcher.solveSymbol(id, "__ZN27AMDRadeonX5000_AMDHWDisplayC2EPK11OSMetaClass", slide, size, true);
    PANIC_COND(constants.constructor == 0, "HWDisplay", "Failed to solve constructor");
}
