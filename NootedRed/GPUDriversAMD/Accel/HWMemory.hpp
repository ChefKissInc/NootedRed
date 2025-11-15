#pragma once
#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/ObjectField.hpp>

class AMDRadeonX5000_AMDHWMemory {
    struct Constants {
        ObjectField<UInt64 (*)(AMDRadeonX5000_AMDHWMemory *)> vtGetVisibleSize {};

        Constants() { this->vtGetVisibleSize = currentKernelVersion() == MACOS_15 ? 0x1F8 : 0x1D8; }
    };

    static Constants constants;

    public:
    UInt64 getVisibleSize() { return constants.vtGetVisibleSize(getMember<void *>(this, 0))(this); }
};
