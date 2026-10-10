// Version-independent interface to the `AMDRadeonX5000_AMDHWInterface` class
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
#include <GPUDriversAMD/Accel/HWAlignManager.hpp>
#include <GPUDriversAMD/Accel/HWMemory.hpp>
#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/ObjectField.hpp>

class AMDRadeonX5000_AMDHWInterface
{
    struct Constants
    {
        ObjectField<void* (*)(AMDRadeonX5000_AMDHWInterface*)>                             vtGetHWInfo;
        ObjectField<AMDRadeonX5000_AMDHWAlignManager* (*)(AMDRadeonX5000_AMDHWInterface*)> vtGetHWAlignManager;
        ObjectField<AMDRadeonX5000_AMDHWMemory* (*)(AMDRadeonX5000_AMDHWInterface*)>       vtGetHWMemory;
        ObjectField<UInt32>                                                                hwInfoAddrConfig;

        Constants()
        {
            if (currentKernelVersion() >= MACOS_11) {
                this->vtGetHWInfo         = 0x1C0;
                this->vtGetHWAlignManager = 0x300;
                this->vtGetHWMemory       = 0x2D8;
                this->hwInfoAddrConfig    = 0xA0;
            }
            else if (currentKernelVersion() <= MACOS_10_14_X) {
                this->vtGetHWInfo         = 0x1B0;
                this->vtGetHWAlignManager = 0x2D8;
                this->vtGetHWMemory       = 0x2B8;
                this->hwInfoAddrConfig    = 0x48;
            }
            else {
                this->vtGetHWInfo         = 0x1A8;
                this->vtGetHWAlignManager = 0x2F8;
                this->vtGetHWMemory       = 0x2D8;
                this->hwInfoAddrConfig    = 0x68;
            }
        }
    };

    static Constants constants;

public:
    auto getHWInfo() { return constants.vtGetHWInfo(getMember<void*>(this, 0))(this); }
    auto getHWMemory() { return constants.vtGetHWMemory(getMember<void*>(this, 0))(this); }
    auto getHWAlignManager() { return constants.vtGetHWAlignManager(getMember<void*>(this, 0))(this); }

    auto getAddrConfig() { return constants.hwInfoAddrConfig(this->getHWInfo()); }
};
