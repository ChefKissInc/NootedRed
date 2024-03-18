//! Copyright © 2022-2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include <IOKit/IOLib.h>
#include <IOKit/IOTypes.h>

void *ZLibAlloc(void *opaque, UInt32 items, UInt32 size);
void ZLibFree(void *opaque, void *ptr);
