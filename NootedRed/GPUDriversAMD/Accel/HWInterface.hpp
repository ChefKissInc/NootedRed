// Version-independent interface to the `AMDRadeonX5000_AMDHWInterface` class
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/Accel/HWAlignManager.hpp>
#include <GPUDriversAMD/Accel/HWMemory.hpp>
#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/ObjectField.hpp>

class AMDRadeonX5000_AMDHWInterface {
    struct Constants {
        ObjectField<void *(*)(AMDRadeonX5000_AMDHWInterface *)> vtGetHWInfo {};
        ObjectField<AMDRadeonX5000_AMDHWAlignManager *(*)(AMDRadeonX5000_AMDHWInterface *)> vtGetHWAlignManager {};
        ObjectField<UInt32> hwInfoAddrConfig {};

        Constants() {
            if (currentKernelVersion() >= MACOS_11) {
                this->vtGetHWInfo = 0x1C0;
                this->vtGetHWAlignManager = 0x300;
                this->hwInfoAddrConfig = 0xA0;
            } else {
                this->vtGetHWInfo = 0x1A8;
                this->vtGetHWAlignManager = 0x2F8;
                this->hwInfoAddrConfig = 0x68;
            }
        }
    };

    static Constants constants;

    public:
    auto getAddrConfig() {
        auto hwInfo = constants.vtGetHWInfo(getMember<void *>(this, 0))(this);
        return constants.hwInfoAddrConfig(hwInfo);
    }

    auto getHWMemory() {
        auto vtable = getMember<void *>(this, 0);
        return getMember<AMDRadeonX5000_AMDHWMemory *(*)(AMDRadeonX5000_AMDHWInterface *)>(vtable, 0x2D8)(this);
    }

    auto getHWAlignManager() { return constants.vtGetHWAlignManager(getMember<void *>(this, 0))(this); }
};
