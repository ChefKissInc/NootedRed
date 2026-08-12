// Decompiled AMDRadeonX6000Framebuffer ATOMBIOS Parser Types
//
// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/ATOMBIOS.hpp>
#include <IOKit/IOTypes.h>

#define _BIOS_ASSERT_MSG(cond) lilu_os_log("ATOM: %s: ASSERT(" #cond ")\n", __PRETTY_FUNCTION__)
#define BIOS_ASSERT(cond, ret)  \
    if (!(cond)) [[unlikely]] { \
        _BIOS_ASSERT_MSG(cond); \
        return ret;             \
    }
#define BIOS_ASSERT_VALID(cond) \
    if (!(cond)) [[unlikely]] { \
        _BIOS_ASSERT_MSG(cond); \
        m_bIsValid = false;     \
    }

typedef struct _AmdAtomFwServicesInit AmdAtomFwServicesInit;

struct AtomDataRevision
{
    UInt32 major;
    UInt32 minor;
};

class AmdAtomFwHelper
{
    union
    {
        struct
        {
            UInt32                                    m_debugLevel;
            char                                      m_pciLocString[0x10];
            class AtiAtomBiosParserService*           m_pBiosParser;
            class AMDRadeonX6000_AmdBiosParserHelper* m_pReference;
            void*                                     m_pReadRom;
            UInt8*                                    m_biosImage;
            UInt32                                    m_biosSize;
        } bigsur;
        struct
        {
            UInt32                          m_debugLevel;
            char                            m_pciLocString[0x10];
            class AtiAtomBiosParserService* m_pBiosParser;
            UInt8*                          m_biosImage;
            UInt32                          m_biosSize;
        } cat;
    };

    AmdAtomFwHelper(AmdAtomFwServicesInit* initInfo);

public:
    virtual ~AmdAtomFwHelper();

    void* getImage(UInt32 imageOffset, UInt32 imageSize) const;
    bool  getAtomDataTableRevision(void* table, AtomDataRevision& dataRevision) const;
    bool  getAtomDataTableRevision(UInt32 tableOffset, AtomDataRevision& dataRevision) const;

    template<typename T>
    inline T* getImage(UInt32 imageOffset) const
    { return static_cast<T*>(getImage(imageOffset, sizeof(T))); }
};

class AmdAtomTableBaseClass
{
protected:
    bool             m_bIsValid;
    AmdAtomFwHelper* m_biosHelper;

    AmdAtomTableBaseClass(AmdAtomFwHelper* biosHelper);

public:
    virtual ~AmdAtomTableBaseClass();
    virtual UInt32 getMajorRevision() const = 0;
    virtual UInt32 getMinorRevision() const = 0;
};

class AmdAtomDataTable : public AmdAtomTableBaseClass
{
protected:
    UInt32           m_tableOffset;
    AtomDataRevision m_tableRevision;

    struct DataTableInitInfo
    {
        AmdAtomFwHelper* biosHelper;
        UInt32           tableOffset;
        AtomDataRevision tableRevision;
    };

    AmdAtomDataTable(DataTableInitInfo* initInfo);

public:
    virtual ~AmdAtomDataTable();
    virtual UInt32 getMajorRevision() const override { return m_tableRevision.major; }
    virtual UInt32 getMinorRevision() const override { return m_tableRevision.minor; }
};

typedef struct _VramModule
{
    UInt32       memorySize;
    UInt32       vramWidth;
    UInt8        extMemoryId;
    ATOMVRAMType memoryType;
    UInt8        numChannels;
    UInt8        channelWidth;
    UInt8        vendorRevId;
    UInt8        vendorId;
} VramModule;

typedef struct _VramInfo
{
    UInt8      _unk[0x7];
    UInt8      vramModuleCount;
    VramModule vramModules[0x10];
} VramInfo;

enum video_memory_type
{
    video_memory_type_unknown = 0x0,
    video_memory_type_gddr5   = 0x2,
    video_memory_type_ddr3    = 0x3,
    video_memory_type_ddr4    = 0x4,
    video_memory_type_hbm     = 0x5,
    video_memory_type_gddr6   = 0x6,
};

typedef struct _VramUsageData
{
    UInt32 startAddressKB;
    UInt16 usedByFirmware;
    UInt16 usedByDriver;
} VramUsageData;

struct AtomFirmwareInfo
{
    UInt32            crystalFreqKHz;
    UInt8             field_4[0x18];
    video_memory_type videoMemoryType;
    UInt32            videoMemoryBitWidth;
    VramUsageData     vramUsageData;
    UInt32            bootDisplayClockKHz;
    UInt8             field_30[0xc];
    UInt32            field_3c;
    UInt32            field_40;
    UInt32            field_44;
    UInt32            field_48;
    UInt32            flags;
    UInt32            fwReservedSizeKB;
    UInt32            field_54;
    UInt32            field_58;
    UInt32            field_5c;
};

class AmdAtomVramInfo : public AmdAtomDataTable
{
protected:
    union
    {
        const ATOMCommonTableHeader* header;
        const ATOMIGPSystemInfoV1*   igpInfoV1;
        const ATOMIGPSystemInfoV2*   igpInfoV2;
        /* dgpu omitted */
    } m_infoTable;

    AmdAtomVramInfo(DataTableInitInfo* initInfo);

public:
    virtual ~AmdAtomVramInfo();
    virtual IOReturn getVramInfo(VramInfo& info) const = 0;

    static video_memory_type translateToVideoMemoryType(ATOMVRAMType vramType);
    IOReturn                 populateVramInfo(AtomFirmwareInfo& info) const;
};
