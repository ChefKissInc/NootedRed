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

    AmdAtomDataTable();
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

enum PSP_DIRECTORY_ENTRY_TYPE
{
    AMD_PUBLIC_KEY              = 0x0,
    PSP_FW_BOOT_LOADER          = 0x1,
    PSP_FW_TRUSTED_OS           = 0x2,
    RECOVERY_PSP_BOOT_LOADER    = 0x3,
    VBIOS_FIRMWARE              = 0x4,
    VBIOS_SIGNATURE             = 0x5,
    SMU_OFF_CHIP_FW             = 0x6,
    AMD_SEC_DBG_PUBLIC_KEY      = 0x7,
    OEM_PSP_FW_PUBLIC_KEY       = 0x8,
    AMD_SOFT_FUSE_CHAIN_0       = 0x9,
    PSP_BOOT_TIME_TRUSTLETS     = 0xa,
    PSP_BOOT_TIME_TRUSTLETS_KEY = 0xb,
    SDMA0_FW                    = 0xc,
    SDMA1_FW                    = 0xd,
    RLCG_FW                     = 0xe,
    RLCV_FW                     = 0xf,
    AMMSCHEDULER_FW             = 0x10,
    MC_FW                       = 0x11,
    SECURITY_GASKET             = 0x12,
    PSP_FW_SYSDRV               = 0x13,
    DBG_UNLOCK_BIN              = 0x14,
    PSP_WRAPPED_IKEK            = 0x21,
    PSP_FW_DIAG_BOOT_LOADER     = 0x23,
    RLC_RESTORE_LIST_GPM_MEM    = 0x2a,
    RLC_RESTORE_LIST_SRM_MEM    = 0x2b,
    RLC_RESTORE_LIST_CNTL       = 0x2c,
    PSP_VBIOS_MODULE            = 0x2d,
    PSP_FW_BOOT_LOADER_STAGE2   = 0x2e,
    PSP_FW_BOOT_LOADER_STAGE3   = 0x2f,
    DXIO_OR_WAFL_PHY_FW         = 0x30,
    PSP_DPPHY_FW                = 0x31,
    PSP_DXIO_FW                 = 0x32,
    PSP_VBIOS_MODULE_STAGE2     = 0x33,
    PSP_VBIOS_MODULE_STAGE3     = 0x34,
    PSP_PLATFORM_CONFIG         = 0x35,
    PSP_SEC_POLICY_STAGE2       = 0x36,
    PSP_IP_DISCOVERY_BIN        = 0x37,
    VBIOS_PUBLIC_KEY_TOKEN      = 0x38,
    WAFL_INIT                   = 0x39,
    DMCU_ERAM_FW                = 0x3a,
    DMCU_ISR_FW                 = 0x3b,
    PCIE_ESM_MODULE             = 0x3c,
    PSP_KEY_DATABASE            = 0x3d,
    PSP_WHITELIST               = 0x3e,
    PSPOS_KEY_DATABASE          = 0x3f,
    RAS_MODULE                  = 0x40,
    DF_TOPOLOGY_TABLE           = 0x41,
    ANTI_ROLLBACK_TABLE         = 0x42,
    PSP_IP_DISCOVERY_BIN1       = 0x43,
    BIST_TRAINING_DATA          = 0x44,
    MPIO_FW                     = 0x46,
    USBCPD_MODULE               = 0x47,
    BOARDCONFIG_MODULE          = 0x49,
    UMC_STG2_FW                 = 0x4a,
    VBL_MGPU_MODULE             = 0x4b,
    SPIROM_INFO_TABLE           = 0x4c,
    TOS_ANTI_ROLLBACK_TABLE     = 0x4d,
    PSP_PENETRATE_MODULE        = 0x5e,
    DMCUB_FW_60                 = 0x60,
    DMCUB_DATA_61               = 0x61,
    DMCUB_FW                    = 0x71,
    DMCUB_DATA                  = 0x72,
    DF_RIB                      = 0x76,
    RAS_MCA_TABLE               = 0x77,
    SPIROM_WRITE                = 0x78,
};

struct PSP_DIRECTORY_ENTRY
{
    PSP_DIRECTORY_ENTRY_TYPE type;
    UInt32                   size;
    UInt32                   location;
    UInt32                   field_c;
};

struct PSP_DIRECTORY
{
    UInt32              cookie;
    UInt32              checksum;
    UInt32              numEntries;
    UInt32              flags;
    PSP_DIRECTORY_ENTRY entries[0x40];
};

struct AtomPspFirmwareInfo
{
    UInt32 mcFwVersion;
    UInt32 smuOffChipFwVersion;
    UInt32 dmcubFwVersion;
    UInt32 pspFwBootLoaderVersion;
    UInt32 vbiosModuleVersion;
};

class AmdAtomPspDirectory : public AmdAtomDataTable
{
protected:
    struct DirectoryTable
    {
        const PSP_DIRECTORY* pspDirectory;
        UInt32               pspDirectorySize;
        UInt32               pspFwBootLoaderRomOff;
        UInt32               pspFwBootLoaderVersion;
        UInt32               pspFwBootLoaderStage2RomOff;
        UInt32               pspFwBootLoaderStage2Version;
        UInt32               pspFwBootLoaderStage3RomOff;
        UInt32               pspFwBootLoaderStage3Version;
        UInt32               pspVBIOSModuleRomOff;
        UInt32               pspVBIOSModuleVersion;
        UInt32               pspVBIOSModuleStage2RomOff;
        UInt32               pspVBIOSModuleStage2Version;
        UInt32               pspVBIOSModuleStage3RomOff;
        UInt32               pspVBIOSModuleStage3Version;
        UInt32               smuOffChipFwRomOff;
        UInt32               smuOffChipFwVersion;
        UInt32               dmcubFwRomOff;
        UInt32               dmcubFwVersion;
        UInt32               mcFwRomOff;
        UInt32               mcFwVersion;
    };

    void*            m_pspDirectoryTable;
    DirectoryTable   m_directoryTable;
    AmdAtomFwHelper* m_biosHelper1;

    AmdAtomPspDirectory();
    AmdAtomPspDirectory(DataTableInitInfo* initInfo);

public:
    virtual ~AmdAtomPspDirectory();
    virtual IOReturn getFirmwareInfo(AtomPspFirmwareInfo& info) const = 0;
};
