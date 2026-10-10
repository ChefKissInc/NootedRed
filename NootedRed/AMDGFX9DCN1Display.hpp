// DCN 1 Display implementation for GFX9
// Derivative of AMDRadeonX5000 and AMDRadeonX6000 decompilation
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
#include <AMDGFX9DCNDisplay.hpp>

class AMDRadeonX5000_AMDGFX9DCN1Display : public AMDRadeonX5000_AMDGFX9DCNDisplay
{
    static VFT vft;

    static void Constructor(AMDRadeonX5000_AMDGFX9DCN1Display* self, const OSMetaClass* metaClass);

    static void initDCNRegOffs(AMDRadeonX5000_AMDGFX9DCN1Display* self);

public:
    PWDeclareRuntimeMC(AMDRadeonX5000_AMDGFX9DCN1Display, Constructor)

    static void resolve(const char* kext);
};
