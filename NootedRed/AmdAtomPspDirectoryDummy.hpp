// AmdAtomPspDirectory Dummy Implementation
//
// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/FB/BiosParser.hpp>

class AmdAtomPspDirectoryDummy : public AmdAtomPspDirectory
{
    AmdAtomPspDirectoryDummy();

public:
    virtual ~AmdAtomPspDirectoryDummy();
    virtual IOReturn getFirmwareInfo(AtomPspFirmwareInfo& info) const override;

    static AmdAtomPspDirectoryDummy* create();
};
