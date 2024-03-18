//! Copyright © 2022-2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "ZlibAlloc.hpp"

void *ZLibAlloc(void *, UInt32 items, UInt32 size) {
    UInt32 allocSize = items * size + sizeof(UInt32);
    auto *mem = static_cast<UInt32 *>(IOMallocZero(allocSize));

    if (!mem) { return nullptr; }

    *mem = allocSize;
    return mem + 1;
}

void ZLibFree(void *, void *ptr) {
    auto *mem = static_cast<UInt32 *>(ptr) - 1;
    IOFree(mem, *mem);
}
