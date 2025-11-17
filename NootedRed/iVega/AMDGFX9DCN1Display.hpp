// DCN 1 Display implementation for GFX9
// Derivative of AMDRadeonX5000 and AMDRadeonX6000 decompilation
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <iVega/AMDGFX9DCNDisplay.hpp>

class AMDRadeonX5000_AMDGFX9DCN1Display : public AMDRadeonX5000_AMDGFX9DCNDisplay {
    static RuntimeVFT<vftCount, 1> vft;

    static void Constructor(AMDRadeonX5000_AMDGFX9DCN1Display *self, const OSMetaClass *metaClass);

    static void initDCNRegOffs(AMDRadeonX5000_AMDGFX9DCN1Display *self);

    public:
    PWDeclareRuntimeMC(AMDRadeonX5000_AMDGFX9DCN1Display, Constructor);

    static void resolve(const char *const kext);
};
