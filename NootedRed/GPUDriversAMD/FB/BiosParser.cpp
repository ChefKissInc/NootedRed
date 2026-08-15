// Decompiled AMDRadeonX6000Framebuffer ATOMBIOS Parser Types
//
// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <GPUDriversAMD/FB/BiosParser.hpp>
#include <PenguinWizardry/KernelVersion.hpp>

void* AmdAtomFwHelper::getImage(const UInt32 imageOffset, const UInt32 imageSize) const
{
    if (currentKernelVersion() >= MACOS_11) {
        if (imageOffset + imageSize > bigsur.m_biosSize) { return nullptr; }
        return &bigsur.m_biosImage[imageOffset];
    }
    if (imageOffset + imageSize > cat.m_biosSize) { return nullptr; }
    return &cat.m_biosImage[imageOffset];
}

bool AmdAtomFwHelper::getAtomDataTableRevision(void* const table, AtomDataRevision& dataRevision) const
{
    dataRevision.major = 0;
    dataRevision.minor = 0;

    if (table == nullptr) [[unlikely]] { return false; }

    const auto header  = static_cast<const ATOMCommonTableHeader*>(table);
    dataRevision.major = header->formatRev & 0x3F;
    dataRevision.minor = header->contentRev & 0x3F;

    return true;
}

bool AmdAtomFwHelper::getAtomDataTableRevision(const UInt32 tableOffset, AtomDataRevision& dataRevision) const
{
    if (tableOffset == 0) [[unlikely]] { return false; }

    return getAtomDataTableRevision(getImage<ATOMCommonTableHeader>(tableOffset), dataRevision);
}

AmdAtomTableBaseClass::AmdAtomTableBaseClass(AmdAtomFwHelper* const biosHelper)
{
    m_bIsValid   = true;
    m_biosHelper = biosHelper;
}

AmdAtomTableBaseClass::~AmdAtomTableBaseClass() { }

AmdAtomDataTable::AmdAtomDataTable() :
    AmdAtomTableBaseClass(nullptr)
{
    m_tableOffset = 0;
    memset(&m_tableRevision, 0, sizeof(m_tableRevision));
}

AmdAtomDataTable::AmdAtomDataTable(DataTableInitInfo* const initInfo) :
    AmdAtomTableBaseClass(initInfo->biosHelper),
    m_tableOffset(initInfo->tableOffset),
    m_tableRevision(initInfo->tableRevision)
{ BIOS_ASSERT_VALID(m_tableOffset != 0); }

AmdAtomDataTable::~AmdAtomDataTable() { }

AmdAtomVramInfo::AmdAtomVramInfo(DataTableInitInfo* const initInfo) :
    AmdAtomDataTable(initInfo)
{ }

AmdAtomVramInfo::~AmdAtomVramInfo() { }

video_memory_type AmdAtomVramInfo::translateToVideoMemoryType(ATOMVRAMType vramType)
{
    switch (vramType) {
        case kATOMVRAMTypeGDDR6: return video_memory_type_gddr6;
        case kATOMVRAMTypeGDDR5: return video_memory_type_gddr5;
        case kATOMVRAMTypeHBM2 :
        case kATOMVRAMTypeHBM2E:
        case kATOMVRAMTypeHBM3 :    // These two are actually missing from the decomp.
        case kATOMVRAMTypeHBM3E: return video_memory_type_hbm;
        case kATOMVRAMTypeDDR3 :    // `AMDRadeonX6000_AmdBiosParserHelper::readFirmwareInfo` will throw up on DDR3.
        case kATOMVRAMTypeDDR4 : return video_memory_type_ddr4;
        default                : return video_memory_type_unknown;
    }
}

IOReturn AmdAtomVramInfo::populateVramInfo(AtomFirmwareInfo& fwInfo) const
{
    VramInfo info;
    memset(&info, 0, sizeof(info));
    const auto err = getVramInfo(info);
    BIOS_ASSERT(err == kIOReturnSuccess, err);

    fwInfo.videoMemoryBitWidth = info.vramModules[0].vramWidth;
    fwInfo.videoMemoryType     = translateToVideoMemoryType(info.vramModules[0].memoryType);

    return kIOReturnSuccess;
}

AmdAtomPspDirectory::AmdAtomPspDirectory() { }

AmdAtomPspDirectory::~AmdAtomPspDirectory() { }
