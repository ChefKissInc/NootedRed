// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_patcher.hpp>
#include <PrivateHeaders/ObjectField.hpp>

namespace iVega {
    class X6000 {
        friend class X5000;

        bool initialised {false};
        ObjectField<UInt32> regBaseField {};
        mach_vm_address_t orgAllocateAMDHWDisplay {0};
        mach_vm_address_t orgInitDCNRegistersOffsets {0};

        public:
        static X6000 &singleton();

        void init();

        private:
        void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

        static bool wrapAccelStartX6000();
        static void wrapInitDCNRegistersOffsets(void *that);
    };
};    // namespace iVega
