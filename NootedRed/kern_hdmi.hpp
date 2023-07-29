//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include "kern_amd.hpp"
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>

class AppleGFXHDA {
    public:
    static AppleGFXHDA *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);
};
