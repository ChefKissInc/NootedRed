// AmdAtomVramInfo IGP implementation
//
// Copyright (C) 2026  ChefKiss.
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
#include <GPUDriversAMD/FB/BiosParser.hpp>

class AmdAtomVramInfoIGP : public AmdAtomVramInfo
{
protected:
    AmdAtomVramInfoIGP(DataTableInitInfo* initInfo);

    static UInt32       translateMemoryTypeWidth(ATOMDMIT17MemType memType);
    static ATOMVRAMType translateMemoryType(ATOMDMIT17MemType memType);

    static void fillVramInfo(VramInfo& info, UInt8 channelCount, ATOMDMIT17MemType memoryType);

public:
    virtual ~AmdAtomVramInfoIGP();

    static AmdAtomVramInfoIGP* createVramInfoIGP(AmdAtomFwHelper* biosHelper, UInt32 tableOffset);
};

class AmdAtomVramInfoIGPV1 : public AmdAtomVramInfoIGP
{
public:
    AmdAtomVramInfoIGPV1(DataTableInitInfo* initInfo);

    virtual ~AmdAtomVramInfoIGPV1();
    virtual IOReturn getVramInfo(VramInfo& info) const override;
};

class AmdAtomVramInfoIGPV2 : public AmdAtomVramInfoIGP
{
public:
    AmdAtomVramInfoIGPV2(DataTableInitInfo* initInfo);

    virtual ~AmdAtomVramInfoIGPV2();
    virtual IOReturn getVramInfo(VramInfo& info) const override;
};
