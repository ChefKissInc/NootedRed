// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once

enum AMDHWEngineType {
    kAMDHWEngineTypePM4 = 0,
    kAMDHWEngineTypeSDMA0,
    kAMDHWEngineTypeSDMA1,
    kAMDHWEngineTypeSDMA2,
    kAMDHWEngineTypeSDMA3,
    kAMDHWEngineTypeUVD0,
    kAMDHWEngineTypeUVD1,
    kAMDHWEngineTypeVCE,
    kAMDHWEngineTypeVCN0,
    kAMDHWEngineTypeVCN1,
    kAMDHWEngineTypeSAMU,
    kAMDHWEngineTypeMax,
};
static_assert(sizeof(AMDHWEngineType) == 0x4);
