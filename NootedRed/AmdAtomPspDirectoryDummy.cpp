//
//  AmdAtomPspDirectoryDummy.cpp
//  NootedRed
//
//  Created by Visual on 15/8/26.
//  Copyright © 2026 ChefKiss. All rights reserved.
//

#include "AmdAtomPspDirectoryDummy.hpp"

AmdAtomPspDirectoryDummy::AmdAtomPspDirectoryDummy() { }

AmdAtomPspDirectoryDummy::~AmdAtomPspDirectoryDummy() { }

IOReturn AmdAtomPspDirectoryDummy::getFirmwareInfo(AtomPspFirmwareInfo& info) const
{
    memset(&info, 0, sizeof(info));
    return kIOReturnSuccess;
}

AmdAtomPspDirectoryDummy* AmdAtomPspDirectoryDummy::create() { return new AmdAtomPspDirectoryDummy; }
