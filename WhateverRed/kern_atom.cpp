#include "kern_atom.hpp"

OSDefineMetaClassAndStructors(AtiAtomTable, OSObject);

bool AtiAtomTable::init(void *helper) {
    if (!OSObject::init())
        return false;

    this->helper = helper;

    return true;
}

OSDefineMetaClassAndStructors(AtiDataTable, AtiAtomTable);

bool AtiDataTable::init(DataTableInitInfo *initInfo) {
    if (!AtiAtomTable::init(initInfo->helper))
        return false;

    this->tableOffset = initInfo->tableOffset;
    this->revision = initInfo->revision;

    return true;
}

uint32_t AtiDataTable::getMajorRevision() {
    return this->revision.formatRevision;
}

uint32_t AtiDataTable::getMinorRevision() {
    return this->revision.contentRevision;
}

OSDefineMetaClassAndStructors(IntegratedVRAMInfoInterface, AtiDataTable);

void IntegratedVRAMInfoInterface::debugVramInfo() {
    IOLog("RAD: Get ready to move your fingor!");
}
