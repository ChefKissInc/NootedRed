// Version-independent interface to the `AMDRadeonX5000_AMDHWDisplay` class
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/Accel/HWInterface.hpp>
#include <GPUDriversAMD/Accel/HWRegisters.hpp>
#include <GPUDriversAMD/FB/FramebufferInfo.hpp>
#include <Headers/kern_patcher.hpp>
#include <IOKit/graphics/IOFramebuffer.h>
#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/RuntimeVFT.hpp>

enum struct AMDFlipOption : UInt8
{
    Registers,
    DCN2,
    DCN3,
};

typedef struct _AMD_SET_REG_ENTRY_
{
    UInt32 offset;
    UInt32 value;
} AMD_SET_REG_ENTRY;

typedef struct _AMD_SET_REG_
{
    UInt32            numEntries;
    UInt32            field_4;    // ???? seemingly unused
    AMD_SET_REG_ENTRY entries[11];
} AMD_SET_REG;

enum struct AMDHWSurfaceFlipType
{
    VSync,
    Immediate,
    HSync,
};

enum struct AMDSwapInterval
{
    Immediate,
    VSync,
};

enum struct AMDHWSurfacePixelFormat
{
    ARGB1555,
    ABGR1555,
    RGB565,
    BGR565,
    ARGB8888,
    ABGRA8888,
    ARGB2101010,
    ABGR2101010,
    BGRA1010102,
    RGBA1010102,
    ARGB16161616,
    BGRA16161616,
    RGBA16161616,
    ARGB16161616F,
    ABGR16161616F,
};

enum struct AMDHWRotationAngle
{
    DEG_0,
    DEG_90,
    DEG_180,
    DEG_270,
    Undefined = 0xFF,
};

struct AMDPipeRect
{
    float x;
    float y;
    float width;
    float height;
};

enum struct AMDHWSurfaceDCCIndBlk : UInt8
{
    Unconstrained,
    BLK_64B,
    BLK_128B,
    BLK_64B_NO_128BCL_S,
};

struct AMDHWSurfaceTilingParamsDCN2
{
    UInt32 numPipes;
    UInt32 numBanks;
    UInt32 pipeInter;
    UInt32 numShaderEngines;
    UInt32 numRbPerSe;
    UInt32 maxFrags;
    UInt32 swizzleMode;
    UInt32 resourceType;    // unused?
    UInt32 shaderEn;

    bool operator==(AMDHWSurfaceTilingParamsDCN2& other) const { return memcmp(this, &other, sizeof(*this)) == 0; }
    bool operator!=(AMDHWSurfaceTilingParamsDCN2& other) const { return !(*this == other); }
};

struct AMDHWSurfaceTilingParamsDCN3
{
    UInt32 numPipes;
    UInt32 pipeInterleave;
    UInt32 maxCompressedFrags;
    UInt32 numPackers;
    UInt32 swizzleMode;
    UInt32 metaLinear;
    UInt32 pipeAligned;

    bool operator==(AMDHWSurfaceTilingParamsDCN3& other) const { return memcmp(this, &other, sizeof(*this)) == 0; }
    bool operator!=(AMDHWSurfaceTilingParamsDCN3& other) const { return !(*this == other); }
};

struct AMDHWSurfaceDCCParams
{
    UInt64                metaAddress;
    UInt64                constantColor;
    UInt32                metaPitch;
    bool                  enabled;
    AMDHWSurfaceDCCIndBlk indBlockSize;
};

union AMDFlipParam
{
    AMD_SET_REG reg;    // AMDFlipOption::REGISTERS
    struct
    {
        UInt64                  surfaceAddr;
        AMDHWSurfaceFlipType    flipType;
        UInt32                  surfacePitch;
        AMDHWSurfacePixelFormat surfaceFormat;
        AMDHWRotationAngle      surfaceRotation;
        AMDPipeRect             surfaceRect;
        union
        {
            AMDHWSurfaceTilingParamsDCN2 dcn2;    // AMDFlipOption::DCN2
            AMDHWSurfaceTilingParamsDCN3 dcn3;    // AMDFlipOption::DCN3
        } tilingParams;
        AMDHWSurfaceDCCParams dccParams;
    } dcn;
};

struct AMDPipeFlip
{
    UInt32        pixelFormat;
    UInt8         rotation;
    AMDFlipOption flipOption;
    UInt16        protectionOption;
    AMDFlipParam  flipParam;
    UInt8         _unk[0x1B0];
};

enum struct CRTHWDepth
{
    DEPTH_8  = 0,
    DEPTH_16 = 1,
    DEPTH_32 = 2,
    DEPTH_64 = 3,
};

inline const char* stringifyCRTHWDepth(CRTHWDepth v)
{
    switch (v) {
        case CRTHWDepth::DEPTH_8 : return "DEPTH_8";
        case CRTHWDepth::DEPTH_16: return "DEPTH_16";
        case CRTHWDepth::DEPTH_32: return "DEPTH_32";
        case CRTHWDepth::DEPTH_64: return "DEPTH_64";
    }
}

enum struct CRTHWFormat
{
    FORMAT_8  = 0,
    FORMAT_10 = 1,
    FORMAT_12 = 2,
};

inline const char* stringifyCRTHWFormat(CRTHWFormat v)
{
    switch (v) {
        case CRTHWFormat::FORMAT_8 : return "FORMAT_8";
        case CRTHWFormat::FORMAT_10: return "FORMAT_10";
        case CRTHWFormat::FORMAT_12: return "FORMAT_12";
    }
}

struct AMDHWDisplayState
{
    class Status
    {
        UInt32 bits{0};

        static constexpr UInt8 IS_ENABLED_SHIFT    = 0;
        static constexpr UInt8 IS_INTERLACED_SHIFT = 1;
        static constexpr UInt8 IS_BUILT_IN_SHIFT   = 2;

        static constexpr UInt8 IS_FULLSCREEN_ENABLED_SHIFT = 5;    // Only <=12

        struct Constants
        {
            UInt8 isAccelBackedShift;
            UInt8 isIOFBFlipEnabledShift;
            UInt8 isWSAASupportedShift;
            UInt8 isDPTSupportedShift;

            Constants()
            {
                if (currentKernelVersion() >= MACOS_13) {
                    this->isAccelBackedShift     = 5;
                    this->isIOFBFlipEnabledShift = 6;
                    this->isWSAASupportedShift   = 7;
                    this->isDPTSupportedShift    = 8;
                }
                else {
                    this->isAccelBackedShift     = 6;
                    this->isIOFBFlipEnabledShift = 7;
                    this->isWSAASupportedShift   = 8;
                    this->isDPTSupportedShift    = 9;
                }
            }
        };

        static Constants constants;

        constexpr Status(const UInt32 bits) :
            bits{bits}
        { }

    public:
        constexpr Status() { }

        constexpr Status operator|(const Status& other) const { return Status(this->bits | other.bits); }

        constexpr Status& operator|=(const Status& other)
        {
            (this->bits |= other.bits);
            return *this;
        }

#define GET_SET_(_N, _S)                                                 \
    bool is##_N() const { return ((this->bits >> (_S)) & 1) != 0; }      \
                                                                         \
    void setIs##_N(const bool value)                                     \
    {                                                                    \
        if (value) { this->bits |= static_cast<UInt32>(value) << (_S); } \
        else {                                                           \
            this->bits &= ~getBit<UInt32>(_S);                           \
        }                                                                \
    }

#define GET_SET_ALL(_N, _S) GET_SET_(_N, getBit<UInt32>(IS_##_S##_SHIFT))
#define GET_SET(_N)         GET_SET_(_N, constants.is##_N##Shift)

        GET_SET_ALL(Enabled, ENABLED)
        GET_SET_ALL(Interlaced, INTERLACED)
        GET_SET_ALL(BuiltIn, BUILT_IN)
        GET_SET(AccelBacked)
        GET_SET(IOFBFlipEnabled)
        GET_SET(WSAASupported)
        GET_SET(DPTSupported)

#undef GET_SET_
#undef GET_SET_ALL
#undef GET_SET
    } status;
    struct AMDRadeonX6000_AMDAccelResource* resource;
    void*                                   field10;
    IOFramebuffer*                          framebuffer;
    IOPixelInformation                      pixelInfo;
};

struct AMDVRRTimestampInfo
{
    UInt32 field0;    // ????
    UInt32 lastTransactionTimestamp;
    UInt32 currentFrameStartTimestamp;
    UInt32 lastTransactionStartTime;
    UInt32 currentFrameVTotal;
    UInt32 horizontalLineTime;
    UInt32 currentFrameTime;
    UInt32 vTotalMin;
    UInt32 vTotalMax;
    UInt32 transactionOnGlassTime;
};

struct AMDVRRTimestampInfoVentura
{
    UInt64 lastTransactionTimestamp;
    UInt64 currentFrameStartTimestamp;
    UInt64 lastTransactionStartTime;
    UInt64 currentFrameVTotal;
    UInt64 horizontalLineTime;
    UInt64 currentFrameTime;
    UInt64 vTotalMin;
    UInt64 vTotalMax;
    UInt64 transactionOnGlassTime;
};

struct ATIFEDSParamInfo2
{
    UInt32 crtIndex;
    UInt32 scaledW;
    UInt32 scaledH;
    UInt32 srcW;
    UInt32 srcH;
    UInt32 scaledFlags;
    UInt32 scaledRot;
    UInt32 field1c;
};

struct CRTHWSpecificInfo
{
    CRTHWDepth   graphDepth;
    CRTHWFormat  graphFormat;
    UInt32       bytesPerPixel;
    ATIPixelMode pixelMode;
    ATIFormat    format;
    bool         isInterlaced;
};

class AMDRadeonX5000_AMDHWDisplay
{
public:
    static constexpr UInt32 MAX_SUPPORTED_DISPLAYS = 6;

private:
    struct Constants
    {
        RuntimeVFTBase                              vft;
        bool                                        (*init)(AMDRadeonX5000_AMDHWDisplay&, void*, void*){nullptr};
        mach_vm_address_t                           constructor{0};
        ObjectField<UInt32[MAX_SUPPORTED_DISPLAYS]> scalerFlags;
        ObjectField<bool>                           isDCN;
        ObjectField<UInt32[MAX_SUPPORTED_DISPLAYS]> wsaaAttributes;
        ObjectField<UInt32 (*)(AMDRadeonX5000_AMDHWDisplay*, UInt32, UInt32*)>      vftGetMirroredDisplays;
        static constexpr inline ObjectField<void (*)(AMDRadeonX5000_AMDHWDisplay*)> vftInitializeRegisters{0x118};
        static constexpr inline ObjectField<void (*)(AMDRadeonX5000_AMDHWDisplay*)> vftRestoreRegisters{0x120};
        static constexpr inline ObjectField<bool (*)(AMDRadeonX5000_AMDHWDisplay*, UInt32, bool, bool, IOFramebuffer*,
                                                     FramebufferInfo*)>
                                                                            vftGetDisplayInfo{0x130};
        ObjectField<UInt64 (*)(AMDRadeonX5000_AMDHWDisplay*, UInt32)>       vftGetCurrentDisplayOffset;
        ObjectField<void (*)(AMDRadeonX5000_AMDHWDisplay*, UInt32, UInt64)> vftSetCurrentDisplayOffset;
        ObjectField<UInt32 (*)(AMDRadeonX5000_AMDHWDisplay*, UInt32*, UInt32, SInt32&, SInt32&, bool, bool)>
                                                                                             vftWriteWaitForVLine;
        ObjectField<void (*)(AMDRadeonX5000_AMDHWDisplay*, UInt32, AMDSwapInterval)>         vftSetFlipControlRegister;
        ObjectField<bool (*)(AMDRadeonX5000_AMDHWDisplay*, void*, void*)>                    vftInit;
        ObjectField<ATIPixelMode (*)(AMDRadeonX5000_AMDHWDisplay*, CRTHWDepth, CRTHWFormat)> vftGetPixelMode;
        ObjectField<ATIFormat (*)(AMDRadeonX5000_AMDHWDisplay*, ATIPixelMode)>               vftGetPixelFormat;
        ObjectField<bool (*)(AMDRadeonX5000_AMDHWDisplay*, AMDPipeFlip*, UInt32, UInt64, UInt64, AMDSwapInterval,
                             UInt32, const AMDTilingInfo*, ATIFormat, UInt32, AMDHWRotationAngle*, AMDPipeFlip*)>
            vftWriteFlipParameters;
        ObjectField<void (*)(AMDRadeonX5000_AMDHWDisplay*, UInt32, UInt32*, UInt32*)>
            vftGetDisplayModeViewportSpecificInfo;
        ObjectField<UInt32 (*)(AMDRadeonX5000_AMDHWDisplay*, UInt32, UInt32*, AMDSwapInterval, UInt64, UInt64)>
                                                                     vftWriteFlipControlRegisters;
        ObjectField<bool (*)(AMDRadeonX5000_AMDHWDisplay*, UInt32)>  vftIsDisplayControlEnabled;
        ObjectField<bool (*)(AMDRadeonX5000_AMDHWDisplay*, UInt32)>  vftIsDisplayInterlaceEnabled;
        ObjectField<bool (*)(AMDRadeonX5000_AMDHWDisplay*, UInt32)>  vftIsFlipPending;
        ObjectField<AMDFlipOption (*)(AMDRadeonX5000_AMDHWDisplay*)> vftGetFlipOption;
        ObjectField<UInt32 (*)(AMDRadeonX5000_AMDHWDisplay*)>        vftGetNumberOfSupportedDisplays;

        Constants()
        {
            if (currentKernelVersion() <= MACOS_10_14_X) {
                this->vftGetCurrentDisplayOffset            = 0x220;
                this->vftSetCurrentDisplayOffset            = 0x228;
                this->vftWriteWaitForVLine                  = 0x250;
                this->vftSetFlipControlRegister             = 0x258;
                this->vftInit                               = 0x288;
                this->vftGetPixelMode                       = 0x298;
                this->vftGetPixelFormat                     = 0x2A0;
                this->vftWriteFlipParameters                = 0x2B8;
                this->vftGetDisplayModeViewportSpecificInfo = 0x2C8;
                this->vftWriteFlipControlRegisters          = 0x2D0;
                this->vftIsDisplayControlEnabled            = 0x2E8;
                this->vftIsDisplayInterlaceEnabled          = 0x2F0;
                this->vftIsFlipPending                      = 0x2F8;
                this->vftGetNumberOfSupportedDisplays       = 0x1F8;
            }
            else {
                if (currentKernelVersion() >= MACOS_13) {
                    this->vftWriteWaitForVLine                  = 0x1C8;
                    this->vftInit                               = 0x1F8;
                    this->vftGetPixelMode                       = 0x210;
                    this->vftGetPixelFormat                     = 0x218;
                    this->vftWriteFlipParameters                = 0x230;
                    this->vftGetDisplayModeViewportSpecificInfo = 0x238;
                    this->vftIsDisplayControlEnabled            = 0x248;
                    this->vftIsDisplayInterlaceEnabled          = 0x250;
                    this->vftGetFlipOption                      = 0x258;
                }
                else {
                    if (currentKernelVersion() >= MACOS_11) {
                        this->vftInit          = 0x278;
                        this->vftGetFlipOption = 0x2E8;
                    }
                    else {
                        this->vftInit = 0x280;
                    }

                    this->vftGetCurrentDisplayOffset            = 0x210;
                    this->vftSetCurrentDisplayOffset            = 0x218;
                    this->vftSetFlipControlRegister             = 0x248;
                    this->vftWriteWaitForVLine                  = 0x240;
                    this->vftGetPixelMode                       = 0x290;
                    this->vftGetPixelFormat                     = 0x298;
                    this->vftWriteFlipParameters                = 0x2B0;
                    this->vftGetDisplayModeViewportSpecificInfo = 0x2B8;
                    this->vftWriteFlipControlRegisters          = 0x2C0;
                    this->vftIsDisplayControlEnabled            = 0x2D0;
                    this->vftIsDisplayInterlaceEnabled          = 0x2D8;
                    this->vftIsFlipPending                      = 0x2E0;
                }
            }

            if (currentKernelVersion() >= MACOS_13) {
                this->scalerFlags            = 0x500;
                this->isDCN                  = 0x518;
                this->wsaaAttributes         = 0x570;
                this->vftGetMirroredDisplays = 0x168;
            }
            else {
                this->isDCN                  = 0x47DC;
                this->scalerFlags            = 0x47C4;
                this->vftGetMirroredDisplays = 0x198;
                if (currentKernelVersion() >= MACOS_11) { this->wsaaAttributes = 0x4810; }
                else {
                    this->wsaaAttributes = 0x47E8;
                }
            }
        }
    };

    auto& vf() { return getMember<void*>(this, 0); }

protected:
    static Constants constants;

public:
    auto& getHWInterface() { return getMember<AMDRadeonX5000_AMDHWInterface*>(this, 0x18); }
    auto& getHWRegisters() { return getMember<AMDRadeonX5000_AMDHWRegisters*>(this, 0x28); }
    auto& displayStates() { return getMember<AMDHWDisplayState*>(this, 0x40); }
    auto& combinedStatus() { return getMember<AMDHWDisplayState::Status>(this, 0x48); }
    auto& fedsParamInfo() { return getMember<ATIFEDSParamInfo2*>(this, 0x50); }
    auto& supportedDisplayCount() { return getMember<UInt32>(this, 0x58); }
    auto& viewportStartYs() { return getMember<UInt32*>(this, 0x68); }
    auto& viewportHeights() { return getMember<UInt32*>(this, 0x70); }
    auto& crtHWSpecificInfos() { return getMember<CRTHWSpecificInfo[MAX_SUPPORTED_DISPLAYS]>(this, 0x7C); }
    auto& surfInfoOutputs() { return getMember<ADDR2_COMPUTE_SURFACE_INFO_OUTPUT[MAX_SUPPORTED_DISPLAYS]>(this, 0xE0); }
    auto& savedSwizzleModes() { return getMember<UInt32[MAX_SUPPORTED_DISPLAYS]>(this, 0x3E0); }
    auto& savedResourceTypes() { return getMember<AddrResourceType[MAX_SUPPORTED_DISPLAYS]>(this, 0x3F8); }
    auto& swizzleModes() { return getMember<UInt32[MAX_SUPPORTED_DISPLAYS]>(this, 0x410); }
    auto& scalerFlags() { return constants.scalerFlags(this); }
    auto& isDCN() { return constants.isDCN(this); }
    auto& vrrTimestampInfo()
    {
        assert(currentKernelVersion() < MACOS_13 && currentKernelVersion() >= MACOS_11);
        return getMember<AMDVRRTimestampInfo>(this, 0x47E0);
    }
    auto& vrrTimestampInfoVentura()
    {
        assert(currentKernelVersion() >= MACOS_13);
        return getMember<AMDVRRTimestampInfoVentura>(this, 0x520);
    }
    auto& wsaaAttributes() { return constants.wsaaAttributes(this); }
    auto  getMirroredDisplays(const UInt32 fbIndex, UInt32* const outBuffer)
    { return constants.vftGetMirroredDisplays(this->vf())(this, fbIndex, outBuffer); }
    auto getDisplayModeViewportSpecificInfo(const UInt32 fbIndex, UInt32* const viewportYStart,
                                            UInt32* const viewportHeight)
    {
        return constants.vftGetDisplayModeViewportSpecificInfo(this->vf())(this, fbIndex, viewportYStart,
                                                                           viewportHeight);
    }
    auto getPixelMode(const CRTHWDepth depth, const CRTHWFormat format)
    { return constants.vftGetPixelMode(this->vf())(this, depth, format); }
    auto getPixelFormat(const ATIPixelMode pixelMode)
    { return constants.vftGetPixelFormat(this->vf())(this, pixelMode); }
    auto isDisplayInterlaceEnabled(const UInt32 fbIndex)
    { return constants.vftIsDisplayInterlaceEnabled(this->vf())(this, fbIndex); }
    auto init(void* const hwInterface, void* const fbParams) { return constants.init(*this, hwInterface, fbParams); }

    static void resolve(KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size);

    static auto        constructor() { return constants.constructor; }
    static const auto& vft() { return constants.vft; }
};
