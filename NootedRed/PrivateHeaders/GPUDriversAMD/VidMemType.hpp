// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

enum VideoMemoryType {
    kVideoMemoryTypeUnknown = 0,
    kVideoMemoryTypeDDR2,
    kVideoMemoryTypeGDDR5,
    kVideoMemoryTypeDDR3,
    kVideoMemoryTypeDDR4,
    kVideoMemoryTypeHBM,
    kVideoMemoryTypeGDDR6,
};
