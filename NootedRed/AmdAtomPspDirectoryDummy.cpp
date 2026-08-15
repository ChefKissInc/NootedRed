// AmdAtomPspDirectory Dummy Implementation
//
// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include "AmdAtomPspDirectoryDummy.hpp"

AmdAtomPspDirectoryDummy::AmdAtomPspDirectoryDummy() { }

AmdAtomPspDirectoryDummy::~AmdAtomPspDirectoryDummy() { }

IOReturn AmdAtomPspDirectoryDummy::getFirmwareInfo(AtomPspFirmwareInfo& info) const
{
    memset(&info, 0, sizeof(info));
    return kIOReturnSuccess;
}

AmdAtomPspDirectoryDummy* AmdAtomPspDirectoryDummy::create() { return new AmdAtomPspDirectoryDummy; }
