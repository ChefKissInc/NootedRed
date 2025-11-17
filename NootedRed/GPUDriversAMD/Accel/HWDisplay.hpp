// Version-independent interface to the `AMDRadeonX5000_AMDHWDisplay` class
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/Accel/HWInterface.hpp>
#include <GPUDriversAMD/Accel/HWRegisters.hpp>
#include <Headers/kern_patcher.hpp>
#include <IOKit/graphics/IOFramebuffer.h>
#include <PenguinWizardry/KernelVersion.hpp>

enum struct AMDFlipOption : UInt8 {
    Registers,
    DCN2,
    DCN3,
};
static_assert(sizeof(AMDFlipOption) == 0x1);

struct AMDFlipRegister {
    UInt32 value;
    UInt32 offset;
};
static_assert(sizeof(AMDFlipRegister) == 0x8);

enum struct AMDHWSurfaceFlipType {
    VSync,
    Immediate,
    HSync,
};
static_assert(sizeof(AMDHWSurfaceFlipType) == 0x4);

enum struct AMDSwapInterval {
    Immediate,
    VSync,
};
static_assert(sizeof(AMDSwapInterval) == 0x4);

enum struct AMDHWSurfacePixelFormat {
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
static_assert(sizeof(AMDHWSurfacePixelFormat) == 0x4);

enum struct AMDHWRotationAngle {
    DEG_0,
    DEG_90,
    DEG_180,
    DEG_270,
    Undefined = 0xFF,
};
static_assert(sizeof(AMDHWRotationAngle) == 0x4);

struct AMDPipeRect {
    float x;
    float y;
    float width;
    float height;
};
static_assert(sizeof(AMDPipeRect) == 0x10);

enum struct AMDHWSurfaceDCCIndBlk : UInt8 {
    Unconstrained,
    BLK_64B,
    BLK_128B,
    BLK_64B_NO_128BCL_S,
};
static_assert(sizeof(AMDHWSurfaceDCCIndBlk) == 0x1);

struct AMDHWSurfaceTilingParamsDCN2 {
    UInt32 numPipes;
    UInt32 numBanks;
    UInt32 pipeInter;
    UInt32 numShaderEngines;
    UInt32 numRbPerSe;
    UInt32 maxFrags;
    UInt32 swizzleMode;
    UInt32 field_1c;    // ???? seemingly unused
    UInt32 shaderEn;

    bool operator==(AMDHWSurfaceTilingParamsDCN2 &other) const { return memcmp(this, &other, sizeof(*this)) == 0; }
    bool operator!=(AMDHWSurfaceTilingParamsDCN2 &other) const { return !(*this == other); }
};
static_assert(sizeof(AMDHWSurfaceTilingParamsDCN2) == 0x24);

struct AMDHWSurfaceTilingParamsDCN3 {
    UInt32 numPipes;
    UInt32 pipeInterleave;
    UInt32 maxCompressedFrags;
    UInt32 numPackers;
    UInt32 swizzleMode;
    UInt32 metaLinear;
    UInt32 pipeAligned;

    bool operator==(AMDHWSurfaceTilingParamsDCN3 &other) const { return memcmp(this, &other, sizeof(*this)) == 0; }
    bool operator!=(AMDHWSurfaceTilingParamsDCN3 &other) const { return !(*this == other); }
};
static_assert(sizeof(AMDHWSurfaceTilingParamsDCN3) == 0x1C);

struct AMDHWSurfaceDCCParams {
    UInt64 metaAddress;
    UInt64 constantColor;
    UInt32 metaPitch;
    bool enabled;
    AMDHWSurfaceDCCIndBlk indBlockSize;
};
static_assert(sizeof(AMDHWSurfaceDCCParams) == 0x18);

union AMDFlipParam {
    struct {
        UInt32 numFlipRegister;
        UInt32 field_4;    // ???? seemingly unused
        AMDFlipRegister flipRegisters[11];
    } reg;    // AMDFlipOption::REGISTERS
    struct {
        UInt64 surfaceAddr;
        AMDHWSurfaceFlipType flipType;
        UInt32 surfacePitch;
        AMDHWSurfacePixelFormat surfaceFormat;
        AMDHWRotationAngle surfaceRotation;
        AMDPipeRect surfaceRect;
        union {
            AMDHWSurfaceTilingParamsDCN2 dcn2;    // AMDFlipOption::DCN2
            AMDHWSurfaceTilingParamsDCN3 dcn3;    // AMDFlipOption::DCN3
        } tilingParams;
        AMDHWSurfaceDCCParams dccParams;
    } dcn;
};
static_assert(sizeof(AMDFlipParam) == 0x68);

struct AMDPipeFlip {
    UInt32 pixelFormat;
    UInt8 rotation;
    AMDFlipOption flipOption;
    UInt16 protectionOption;
    AMDFlipParam flipParam;
    UInt8 _unk[0x1B0];
};
static_assert(sizeof(AMDPipeFlip) == 0x220);

enum struct CRTHWDepth {
    DEPTH_8 = 0,
    DEPTH_16 = 1,
    DEPTH_32 = 2,
    DEPTH_64 = 3,
};
static_assert(sizeof(CRTHWDepth) == 0x4);

inline const char *stringifyCRTHWDepth(CRTHWDepth v) {
    switch (v) {
        case CRTHWDepth::DEPTH_8:
            return "DEPTH_8";
        case CRTHWDepth::DEPTH_16:
            return "DEPTH_1";
        case CRTHWDepth::DEPTH_32:
            return "DEPTH_3";
        case CRTHWDepth::DEPTH_64:
            return "DEPTH_6";
    }
}

enum struct CRTHWFormat {
    FORMAT_8 = 0,
    FORMAT_10 = 1,
    FORMAT_12 = 2,
};
static_assert(sizeof(CRTHWFormat) == 0x4);

inline const char *stringifyCRTHWFormat(CRTHWFormat v) {
    switch (v) {
        case CRTHWFormat::FORMAT_8:
            return "FORMAT_8";
        case CRTHWFormat::FORMAT_10:
            return "FORMAT_10";
        case CRTHWFormat::FORMAT_12:
            return "FORMAT_12";
    }
}

struct AMDHWDisplayState {
    class Status {
        UInt32 bits;

        static constexpr UInt8 IS_ENABLED_SHIFT = 0;
        static constexpr UInt8 IS_INTERLACED_SHIFT = 1;
        static constexpr UInt8 IS_BUILT_IN_SHIFT = 2;

        // 12.0 and older
        static constexpr UInt8 IS_FULLSCREEN_ENABLED_SHIFT = 5;
        static constexpr UInt8 IS_ACCEL_BACKED_SHIFT = 6;
        static constexpr UInt8 IS_IOFB_FLIP_ENABLED_SHIFT = 7;
        static constexpr UInt8 IS_WSAA_SUPPORTED_SHIFT = 8;
        static constexpr UInt8 IS_DPT_SUPPORTED_SHIFT = 9;

        // 13.0 and newer
        static constexpr UInt8 IS_ACCEL_BACKED_SHIFT_MAC13 = 5;
        static constexpr UInt8 IS_IOFB_FLIP_ENABLED_SHIFT_MAC13 = 6;
        static constexpr UInt8 IS_WSAA_SUPPORTED_SHIFT_MAC13 = 7;
        static constexpr UInt8 IS_DPT_SUPPORTED_SHIFT_MAC13 = 8;

        struct Constants {
            UInt8 isAccelBackedShift;
            UInt8 isIOFBFlipEnabledShift;
            UInt8 isWSAASupportedShift;
            UInt8 isDPTSupportedShift;

            Constants() {
                if (currentKernelVersion() >= MACOS_13) {
                    this->isAccelBackedShift = IS_ACCEL_BACKED_SHIFT_MAC13;
                    this->isIOFBFlipEnabledShift = IS_IOFB_FLIP_ENABLED_SHIFT_MAC13;
                    this->isWSAASupportedShift = IS_WSAA_SUPPORTED_SHIFT_MAC13;
                    this->isDPTSupportedShift = IS_DPT_SUPPORTED_SHIFT_MAC13;
                } else {
                    this->isAccelBackedShift = IS_ACCEL_BACKED_SHIFT;
                    this->isIOFBFlipEnabledShift = IS_IOFB_FLIP_ENABLED_SHIFT;
                    this->isWSAASupportedShift = IS_WSAA_SUPPORTED_SHIFT;
                    this->isDPTSupportedShift = IS_DPT_SUPPORTED_SHIFT;
                }
            }
        };

        static Constants constants;

        constexpr Status(const UInt32 bits) : bits {bits} {}

        public:
        constexpr Status() : Status(0) {}

        constexpr Status operator|(const Status &other) const { return Status(this->bits | other.bits); }

        constexpr Status &operator|=(const Status &other) {
            (this->bits |= other.bits);
            return *this;
        }

#define _GET_SET(_N, _S)                                            \
    bool is##_N() const { return ((this->bits >> (_S)) & 1) != 0; } \
                                                                    \
    void setIs##_N(const bool value) {                              \
        if (value) {                                                \
            this->bits |= static_cast<UInt32>(value) << (_S);       \
        } else {                                                    \
            this->bits &= ~getBit<UInt32>(_S);                      \
        }                                                           \
    }

#define GET_SET_ALL(_N, _S) _GET_SET(_N, getBit<UInt32>(IS_##_S##_SHIFT))
#define GET_SET(_N)         _GET_SET(_N, constants.is##_N##Shift)

        GET_SET_ALL(Enabled, ENABLED);
        GET_SET_ALL(Interlaced, INTERLACED);
        GET_SET_ALL(BuiltIn, BUILT_IN);
        GET_SET(AccelBacked);
        GET_SET(IOFBFlipEnabled);
        GET_SET(WSAASupported);
        GET_SET(DPTSupported);

#undef _GET_SET
#undef GET_SET_ALL
#undef GET_SET
    } status;
    struct AMDRadeonX6000_AMDAccelResource *resource;
    void *field10;
    IOFramebuffer *framebuffer;
    IOPixelInformation pixelInfo;
};
static_assert(sizeof(AMDHWDisplayState) == 0xD0);

struct AMDVRRTimestampInfo {
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
static_assert(sizeof(AMDVRRTimestampInfo) == 0x28);

struct AMDVRRTimestampInfoVentura {
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
static_assert(sizeof(AMDVRRTimestampInfoVentura) == 0x48);

struct ATIFEDSParamInfo2 {
    UInt32 crtIndex;
    UInt32 scaledW;
    UInt32 scaledH;
    UInt32 srcW;
    UInt32 srcH;
    UInt32 scaledFlags;
    UInt32 scaledRot;
    UInt32 field1c;
};
static_assert(sizeof(ATIFEDSParamInfo2) == 0x20);

struct CRTHWSpecificInfo {
    CRTHWDepth graphDepth;
    CRTHWFormat graphFormat;
    UInt32 bytesPerPixel;
    ATIPixelMode pixelMode;
    ATIFormat format;
    bool isInterlaced;
};
static_assert(sizeof(CRTHWSpecificInfo) == 0x10);

class AMDRadeonX5000_AMDHWDisplay {
    public:
    static constexpr UInt32 MAX_SUPPORTED_DISPLAYS = 6;

    private:
    struct Constants {
        void **vfuncs {nullptr};
        mach_vm_address_t init {0};
        mach_vm_address_t constructor {0};
        ObjectField<UInt32[MAX_SUPPORTED_DISPLAYS]> scalerFlags {};
        ObjectField<bool> isDCN {};
        ObjectField<UInt32[MAX_SUPPORTED_DISPLAYS]> wsaaAttributes {};
        ObjectField<UInt32 (*)(void *, UInt32, UInt32 *)> vftGetMirroredDisplays {};
        UInt32 vftCount {0};

        Constants() {
            if (currentKernelVersion() >= MACOS_13) {
                this->scalerFlags = 0x500;
                this->isDCN = 0x518;
                this->wsaaAttributes = 0x570;
                this->vftGetMirroredDisplays = 0x168;
                this->vftCount = 0x4C;
            } else {
                this->isDCN = 0x47DC;
                this->scalerFlags = 0x47C4;
                this->vftGetMirroredDisplays = 0x198;
                if (currentKernelVersion() >= MACOS_11) {
                    this->wsaaAttributes = 0x4810;
                    this->vftCount = 0x5E;
                } else {
                    this->wsaaAttributes = 0x47E8;
                    this->vftCount = 0x5D;
                }
            }
        }
    };

    static Constants constants;

    public:
    auto getHWInterface() { return getMember<AMDRadeonX5000_AMDHWInterface *>(this, 0x18); }
    auto getHWRegisters() { return getMember<AMDRadeonX5000_AMDHWRegisters *>(this, 0x28); }
    auto displayStates() { return getMember<AMDHWDisplayState *>(this, 0x40); }
    auto &combinedStatus() { return getMember<AMDHWDisplayState::Status>(this, 0x48); }
    auto fedsParamInfo() { return getMember<ATIFEDSParamInfo2 *>(this, 0x50); }
    auto &supportedDisplayCount() { return getMember<UInt32>(this, 0x58); }
    auto viewportStartYs() { return getMember<UInt32 *>(this, 0x68); }
    auto viewportHeights() { return getMember<UInt32 *>(this, 0x70); }
    auto crtHWSpecificInfos() { return getMember<CRTHWSpecificInfo[MAX_SUPPORTED_DISPLAYS]>(this, 0x7C); }
    auto surfInfoOutputs() { return getMember<ADDR2_COMPUTE_SURFACE_INFO_OUTPUT[MAX_SUPPORTED_DISPLAYS]>(this, 0xE0); }
    auto savedSwizzleModes() { return getMember<UInt32[MAX_SUPPORTED_DISPLAYS]>(this, 0x3E0); }
    auto savedResourceTypes() { return getMember<AddrResourceType[MAX_SUPPORTED_DISPLAYS]>(this, 0x3F8); }
    auto swizzleModes() { return getMember<UInt32[MAX_SUPPORTED_DISPLAYS]>(this, 0x410); }
    auto scalerFlags() { return constants.scalerFlags(this); }
    auto &isDCN() { return constants.isDCN(this); }
    auto &vrrTimestampInfo() {
        assert(currentKernelVersion() < MACOS_13);
        return getMember<AMDVRRTimestampInfo>(this, 0x47E0);
    }
    auto &vrrTimestampInfoVentura() {
        assert(currentKernelVersion() >= MACOS_13);
        return getMember<AMDVRRTimestampInfoVentura>(this, 0x520);
    }
    auto wsaaAttributes() { return constants.wsaaAttributes(this); }

    auto getMirroredDisplays(const UInt32 fbIndex, UInt32 *const outBuffer) {
        return constants.vftGetMirroredDisplays(getMember<void *>(this, 0))(this, fbIndex, outBuffer);
    }

    static void resolve(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    static auto vfuncs() { return constants.vfuncs; }
    static auto init() { return constants.init; }
    static auto constructor() { return constants.constructor; }
    static auto vftCount() { return constants.vftCount; }
};
