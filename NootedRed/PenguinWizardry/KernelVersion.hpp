// Apple *OS Version Constants
// Optimised; both major and minor with one comparison.
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
#include <Headers/kern_util.hpp>

namespace PenguinWizardry
{

    class KernelVersion
    {
        const UInt64 combined;

        static constexpr UInt32 MASK = 0xFFFFFFFF;

    public:
        constexpr KernelVersion(const UInt32 major, const UInt32 minor) :
            combined{minor | (static_cast<UInt64>(major) << 32)}
        { }

        constexpr KernelVersion(const UInt32 major) :
            KernelVersion(major, MASK)
        { }

        constexpr auto major() const -> UInt32 { return (this->combined >> 32) & MASK; }
        constexpr auto minor() const -> UInt32 { return this->combined & MASK; }

        constexpr auto operator>(const KernelVersion& other) const { return this->combined > other.combined; }
        constexpr auto operator<(const KernelVersion& other) const { return this->combined < other.combined; }
        constexpr auto operator>=(const KernelVersion& other) const { return this->combined >= other.combined; }
        constexpr auto operator<=(const KernelVersion& other) const { return this->combined <= other.combined; }
        constexpr auto operator==(const KernelVersion& other) const { return this->combined == other.combined; }
        constexpr auto majorMatches(const KernelVersion& other) const { return this->major() == other.major(); }
    };

}    // namespace PenguinWizardry

const PenguinWizardry::KernelVersion& currentKernelVersion();

static constexpr const PenguinWizardry::KernelVersion MACOS_10_4(8, 0);      // macOS Tiger 10.4
static constexpr const PenguinWizardry::KernelVersion MACOS_10_4_X(8);       // macOS Tiger 10.4.x
static constexpr const PenguinWizardry::KernelVersion MACOS_10_5(9, 0);      // macOS Leopard 10.5
static constexpr const PenguinWizardry::KernelVersion MACOS_10_5_X(9);       // macOS Leopard 10.5.x
static constexpr const PenguinWizardry::KernelVersion MACOS_10_6(10, 0);     // macOS Snow Leopard 10.6
static constexpr const PenguinWizardry::KernelVersion MACOS_10_6_X(10);      // macOS Snow Leopard 10.6.x
static constexpr const PenguinWizardry::KernelVersion MACOS_10_7(11, 0);     // macOS Lion 10.7
static constexpr const PenguinWizardry::KernelVersion MACOS_10_7_X(11);      // macOS Lion 10.7.x
static constexpr const PenguinWizardry::KernelVersion MACOS_10_8(12, 0);     // macOS Mountain Lion 10.8
static constexpr const PenguinWizardry::KernelVersion MACOS_10_8_X(12);      // macOS Mountain Lion 10.8.x
static constexpr const PenguinWizardry::KernelVersion MACOS_10_9(13, 0);     // macOS Mavericks 10.9
static constexpr const PenguinWizardry::KernelVersion MACOS_10_9_X(13);      // macOS Mavericks 10.9.x
static constexpr const PenguinWizardry::KernelVersion MACOS_10_10(14, 0);    // macOS Yosemite 10.10
static constexpr const PenguinWizardry::KernelVersion MACOS_10_10_X(14);     // macOS Yosemite 10.10.x
static constexpr const PenguinWizardry::KernelVersion MACOS_10_11(15, 0);    // macOS El Capitan 10.11
static constexpr const PenguinWizardry::KernelVersion MACOS_10_11_X(15);     // macOS El Capitan 10.11.x
static constexpr const PenguinWizardry::KernelVersion MACOS_10_12(16, 0);    // macOS Sierra 10.12
static constexpr const PenguinWizardry::KernelVersion MACOS_10_12_X(16);     // macOS Sierra 10.12.x
static constexpr const PenguinWizardry::KernelVersion MACOS_10_13(17, 0);    // macOS High Sierra 10.13
static constexpr const PenguinWizardry::KernelVersion MACOS_10_13_X(17);     // macOS High Sierra 10.13.x
static constexpr const PenguinWizardry::KernelVersion MACOS_10_14(18, 0);    // macOS Mojave 10.14
static constexpr const PenguinWizardry::KernelVersion MACOS_10_14_X(18);     // macOS Mojave 10.14.x
static constexpr const PenguinWizardry::KernelVersion MACOS_10_15(19, 0);    // macOS Catalina 10.15
static constexpr const PenguinWizardry::KernelVersion MACOS_10_15_X(19);     // macOS Catalina 10.15.x
static constexpr const PenguinWizardry::KernelVersion MACOS_11(20, 0);       // macOS Big Sur 11
static constexpr const PenguinWizardry::KernelVersion MACOS_11_X(20);        // macOS Big Sur 11.x
static constexpr const PenguinWizardry::KernelVersion MACOS_12(21, 0);       // macOS Monterey 12
static constexpr const PenguinWizardry::KernelVersion MACOS_12_X(21);        // macOS Monterey 12.x
static constexpr const PenguinWizardry::KernelVersion MACOS_13(22, 0);       // macOS Ventura 13
static constexpr const PenguinWizardry::KernelVersion MACOS_13_X(22);        // macOS Ventura 13.x
static constexpr const PenguinWizardry::KernelVersion MACOS_13_4(22, 5);     // macOS Ventura 13.4.x
static constexpr const PenguinWizardry::KernelVersion MACOS_14(23, 0);       // macOS Sonoma 14
static constexpr const PenguinWizardry::KernelVersion MACOS_14_X(23);        // macOS Sonoma 14.x
static constexpr const PenguinWizardry::KernelVersion MACOS_14_4(23, 4);     // macOS Sonoma 14.4.x
static constexpr const PenguinWizardry::KernelVersion MACOS_15(24, 0);       // macOS Sequoia 15
static constexpr const PenguinWizardry::KernelVersion MACOS_15_X(24);        // macOS Sequoia 15.x
static constexpr const PenguinWizardry::KernelVersion MACOS_26(25, 0);       // macOS Tahoe 26
static constexpr const PenguinWizardry::KernelVersion MACOS_26_X(25);        // macOS Tahoe 26.x
