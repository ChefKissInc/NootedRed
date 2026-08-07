// AMD Display Surface Info
//
// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

struct AMD_SURFACE_INFO_STRUCT
{
    UInt16 version;
    UInt16 revision;
    UInt16 sizeOf;
    UInt16 bytesPerPixel;
    UInt16 inWidth;
    UInt16 inHeight;
    UInt16 outWidth;
    UInt16 outHeight;
    UInt32 outTilingMode;
};
