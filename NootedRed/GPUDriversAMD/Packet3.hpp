// AMD GPU MicroEngine Packet 3
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

static UInt32 packet3Header(const UInt8 op, const UInt16 payloadDWords) {
    return (3U << 30) | ((static_cast<UInt32>(op) & 0xFF) << 8) | ((static_cast<UInt32>(payloadDWords) & 0x3FFF) << 16);
}

constexpr UInt32 PACKET3_WRITE_DATA = 0x37;
constexpr UInt32 PACKET3_WRITE_DATA_WR_ONE_ADDR = getBit(16);

inline UInt32 write1RegWritePacket(UInt32 *buffer, const UInt32 reg, const UInt32 data) {
    *buffer++ = packet3Header(PACKET3_WRITE_DATA, 3);    // header
    *buffer++ = PACKET3_WRITE_DATA_WR_ONE_ADDR;          // control
    *buffer++ = reg;                                     // addrLow
    *buffer++ = 0;                                       // addrHigh
    *buffer++ = data;                                    // data
    return 5;
}
