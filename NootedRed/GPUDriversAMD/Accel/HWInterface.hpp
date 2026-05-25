// Version-independent interface to the `AMDRadeonX5000_AMDHWInterface` class
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

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
