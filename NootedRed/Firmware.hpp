//! Copyright © 2022-2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include "ZlibAlloc.hpp"
#include <Headers/kern_util.hpp>
#include <libkern/c++/OSData.h>
#include <libkern/zlib.h>

struct FWDescriptor {
    const char *name;
    const UInt8 *data;
    const UInt32 compressedSize;
    const UInt32 uncompressedSize;
};

#define FIRMWARE(name_, data_, compressedSize_, uncompressedSize_) \
    .name = name_, .data = data_, .compressedSize = compressedSize_, .uncompressedSize = uncompressedSize_

extern const struct FWDescriptor firmware[];
extern const size_t firmwareCount;

struct Firmware {
    UInt8 *data;
    UInt32 size;
};

inline const Firmware getFWByName(const char *name) {
    for (size_t i = 0; i < firmwareCount; i++) {
        if (strcmp(firmware[i].name, name)) { continue; }

        UInt32 size = firmware[i].uncompressedSize;
        auto *dest = reinterpret_cast<Bytef *>(IOMallocZero(size));
        z_stream stream = {
            .next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(firmware[i].data)),
            .avail_in = firmware[i].compressedSize,
            .next_out = dest,
            .avail_out = size,
            .zalloc = ZLibAlloc,
            .zfree = ZLibFree,
        };
        PANIC_COND(inflateInit(&stream) != Z_OK, "FW", "Failed to initialise inflate stream");
        auto err = inflate(&stream, Z_FINISH);
        PANIC_COND(err != Z_STREAM_END, "FW", "Failed to decompress '%s': %d", name, err);
        SYSLOG_COND(inflateEnd(&stream) != Z_OK, "FW", "Failed to end inflate stream");
        return Firmware {.data = dest, .size = firmware[i].uncompressedSize};
    }
    PANIC("FW", "'%s' not found", name);
}
