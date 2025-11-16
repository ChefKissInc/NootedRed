// Apple *OS Version Constants
// Optimised; both major and minor with one comparison.
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/New.hpp>

alignas(PenguinWizardry::KernelVersion) static UInt8 _currentKernelVersion[sizeof(PenguinWizardry::KernelVersion)];

[[gnu::constructor(999)]]
static void _GLOBAL_initCurrentKernelVersion() {
    new (_currentKernelVersion)
        PenguinWizardry::KernelVersion {static_cast<UInt32>(version_major), static_cast<UInt32>(version_minor)};
}

const PenguinWizardry::KernelVersion &currentKernelVersion() {
    return *launder(reinterpret_cast<PenguinWizardry::KernelVersion *>(_currentKernelVersion));
}
