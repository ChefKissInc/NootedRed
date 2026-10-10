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

#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/New.hpp>
#include <libkern/OSTypes.h>

alignas(PenguinWizardry::KernelVersion) static UInt8 _currentKernelVersion[sizeof(PenguinWizardry::KernelVersion)];

[[gnu::constructor(101)]]
static void _GLOBAL_initCurrentKernelVersion()
{
    new (_currentKernelVersion)
        PenguinWizardry::KernelVersion{static_cast<UInt32>(version_major), static_cast<UInt32>(version_minor)};
}

const PenguinWizardry::KernelVersion& currentKernelVersion()
{ return *launder(reinterpret_cast<PenguinWizardry::KernelVersion*>(_currentKernelVersion)); }
