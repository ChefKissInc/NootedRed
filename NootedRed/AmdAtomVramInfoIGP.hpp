// AmdAtomVramInfo IGP implementation
//
// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

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
