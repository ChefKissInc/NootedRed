// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_patcher.hpp>

namespace Hotfixes {
    class X6000FB {
        static constexpr UInt32 IOFBRequestControllerEnabled = 0x1B;

        bool initialised {false};
        mach_vm_address_t orgDpReceiverPowerCtrl {0};
        IOReturn (*orgMessageAccelerator)(void *that, UInt32 requestType, void *arg2, void *arg3, void *arg4) {nullptr};
        mach_vm_address_t orgControllerPowerUp {0};

        public:
        static X6000FB &singleton();

        void init();

        private:
        void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

        static void wrapDpReceiverPowerCtrl(void *link, bool power_on);
        static UInt32 wrapControllerPowerUp(void *that);
    };
};    // namespace Hotfixes
