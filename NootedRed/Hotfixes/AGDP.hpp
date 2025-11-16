// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_patcher.hpp>

namespace Hotfixes {
    class AGDP {
        public:
        static AGDP &singleton();

        void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);
    };
};    // namespace Hotfixes
