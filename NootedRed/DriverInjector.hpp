// IOKit Personality Injector
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

#pragma once
#include <Headers/kern_patcher.hpp>
#include <libkern/c++/OSUnserialize.h>

class DriverInjector
{
    struct Driver
    {
        const char* identifier{nullptr};
        OSArray*    personalities{nullptr};

        Driver(const char* identifier, const char* xml, const size_t xml_size) :
            identifier{identifier}
        {
            OSString*   errStr           = nullptr;
            auto* const dataUnserialized = OSUnserializeXML(xml, xml_size, &errStr);

            PANIC_COND(dataUnserialized == nullptr, "DriverInjector", "BUG: Failed to unserialise XML for `%s`: `%s`",
                       identifier, errStr == nullptr ? "(null)" : errStr->getCStringNoCopy());

            this->personalities = OSDynamicCast(OSArray, dataUnserialized);
            PANIC_COND(this->personalities == nullptr, "DriverInjector", "BUG: XML for `%s` is not an array!",
                       identifier);
        }

        template<const size_t N>
        Driver(const char* identifier, const char (&xml)[N]) :
            Driver(identifier, xml, N)
        { }

        ~Driver() { this->personalities->release(); }
    };

    mach_vm_address_t orgAddDrivers{0};
    UInt8             matchedDrivers{0};
    Driver            drivers[4];

public:
    DriverInjector();

    static DriverInjector& singleton();

    void processPatcher(KernelPatcher& patcher);

private:
    static bool wrapAddDrivers(void* self, OSArray* array, const bool doNubMatching);
};
