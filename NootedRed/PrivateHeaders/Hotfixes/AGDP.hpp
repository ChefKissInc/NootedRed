// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_patcher.hpp>

namespace Hotfixes {
    class AGDP {
        bool initialised {false};

        public:
        static AGDP &singleton();

        void init();

        private:
        void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);
    };
};    // namespace Hotfixes
