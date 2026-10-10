// AMD GPU MicroEngine Packet 3
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
#include <Headers/kern_util.hpp>

static UInt32 packet3Header(const UInt8 op, const UInt16 payloadDWords)
{ return (3U << 30) | ((static_cast<UInt32>(op) & 0xFF) << 8) | ((static_cast<UInt32>(payloadDWords) & 0x3FFF) << 16); }

constexpr UInt32 PACKET3_WRITE_DATA             = 0x37;
constexpr UInt32 PACKET3_WRITE_DATA_WR_ONE_ADDR = getBit(16);

inline UInt32 write1RegWritePacket(UInt32* buffer, const UInt32 reg, const UInt32 data)
{
    *buffer++ = packet3Header(PACKET3_WRITE_DATA, 3);    // header
    *buffer++ = PACKET3_WRITE_DATA_WR_ONE_ADDR;          // control
    *buffer++ = reg;                                     // addrLow
    *buffer++ = 0;                                       // addrHigh
    *buffer++ = data;                                    // data
    return 5;
}
