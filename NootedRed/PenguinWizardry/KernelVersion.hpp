// Apple *OS Version Constants
// Optimised; both major and minor with one comparison.
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

namespace PenguinWizardry {
    class KernelVersion {
        const UInt64 combined;

        public:
        constexpr KernelVersion(const UInt32 major, const UInt32 minor)
            : combined {minor | (static_cast<UInt64>(major) << 32)} {}

        constexpr bool operator>(const KernelVersion &other) const { return this->combined > other.combined; }
        constexpr bool operator<(const KernelVersion &other) const { return this->combined < other.combined; }
        constexpr bool operator>=(const KernelVersion &other) const { return this->combined >= other.combined; }
        constexpr bool operator<=(const KernelVersion &other) const { return this->combined <= other.combined; }
        constexpr bool operator==(const KernelVersion &other) const {
            return (this->combined & ~0xFFFFFFFFull) == (other.combined & ~0xFFFFFFFFull);
        }
    };
}    // namespace PenguinWizardry

const PenguinWizardry::KernelVersion &currentKernelVersion();

static constexpr const PenguinWizardry::KernelVersion MACOS_10_4(8, 0);      // macOS Tiger 10.4
static constexpr const PenguinWizardry::KernelVersion MACOS_10_5(9, 0);      // macOS Leopard 10.5
static constexpr const PenguinWizardry::KernelVersion MACOS_10_6(10, 0);     // macOS Snow Leopard 10.6
static constexpr const PenguinWizardry::KernelVersion MACOS_10_7(11, 0);     // macOS Lion 10.7
static constexpr const PenguinWizardry::KernelVersion MACOS_10_8(12, 0);     // macOS Mountain Lion 10.8
static constexpr const PenguinWizardry::KernelVersion MACOS_10_9(13, 0);     // macOS Mavericks 10.9
static constexpr const PenguinWizardry::KernelVersion MACOS_10_10(14, 0);    // macOS Yosemite 10.10
static constexpr const PenguinWizardry::KernelVersion MACOS_10_11(15, 0);    // macOS El Capitan 10.11
static constexpr const PenguinWizardry::KernelVersion MACOS_10_12(16, 0);    // macOS Sierra 10.12
static constexpr const PenguinWizardry::KernelVersion MACOS_10_13(17, 0);    // macOS High Sierra 10.13
static constexpr const PenguinWizardry::KernelVersion MACOS_10_14(18, 0);    // macOS Mojave 10.14
static constexpr const PenguinWizardry::KernelVersion MACOS_10_15(19, 0);    // macOS Catalina 10.15
static constexpr const PenguinWizardry::KernelVersion MACOS_11(20, 0);       // macOS Big Sur 11.0
static constexpr const PenguinWizardry::KernelVersion MACOS_12(21, 0);       // macOS Monterey 12.0
static constexpr const PenguinWizardry::KernelVersion MACOS_13(22, 0);       // macOS Ventura 13.0
static constexpr const PenguinWizardry::KernelVersion MACOS_13_4(22, 5);     // macOS Ventura 13.4
static constexpr const PenguinWizardry::KernelVersion MACOS_14(23, 0);       // macOS Sonoma 14.0
static constexpr const PenguinWizardry::KernelVersion MACOS_14_4(23, 4);     // macOS Sonoma 14.4
static constexpr const PenguinWizardry::KernelVersion MACOS_15(24, 0);       // macOS Sequoia 15.0
static constexpr const PenguinWizardry::KernelVersion MACOS_26(25, 0);       // macOS Tahoe 26.0
