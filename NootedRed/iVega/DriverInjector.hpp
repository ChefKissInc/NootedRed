// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_patcher.hpp>
#include <libkern/c++/OSUnserialize.h>

namespace iVega {
    class DriverInjector {
        struct Driver {
            const char *identifier {nullptr};
            OSArray *personalities {nullptr};

            Driver(const char *identifier, const char *xml, const size_t xml_size) : identifier {identifier} {
                OSString *errStr = nullptr;
                auto *dataUnserialized = OSUnserializeXML(xml, xml_size, &errStr);

                assertf(dataUnserialized != nullptr, "Failed to unserialise XML for `%s`: `%s`", identifier,
                    errStr == nullptr ? "(null)" : errStr->getCStringNoCopy());

                this->personalities = OSRequiredCast(OSArray, dataUnserialized);
            }

            template<const size_t N>
            Driver(const char *identifier, const char (&xml)[N]) : Driver(identifier, xml, N) {}
        };

        mach_vm_address_t orgAddDrivers {0};
        UInt8 matchedDrivers {0};
        Driver drivers[4];

        public:
        DriverInjector();

        static DriverInjector &singleton();

        void processPatcher(KernelPatcher &patcher);

        private:
        static bool wrapAddDrivers(void *self, OSArray *array, const bool doNubMatching);
    };
};    // namespace iVega
