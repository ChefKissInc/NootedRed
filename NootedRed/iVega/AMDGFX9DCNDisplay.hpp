// AMD GFX 9 DCN Display implementation
// Derivative of AMDRadeonX5000 and AMDRadeonX6000 decompilation
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/Accel/HWAlignManager.hpp>
#include <GPUDriversAMD/Accel/HWDisplay.hpp>
#include <GPUDriversAMD/FB/FramebufferInfo.hpp>
#include <PenguinWizardry/RuntimeMC.hpp>

class AMDRadeonX5000_AMDGFX9DCNDisplay : public AMDRadeonX5000_AMDHWDisplay {
    void initDCNRegOffs() {
        auto vtable = getMember<void **>(this, 0);
        reinterpret_cast<void (*)(const void *)>(vtable[vftCount()])(this);
    }

    static void initialiseRegisters(AMDRadeonX5000_AMDGFX9DCNDisplay *self);
    static void restoreRegisters();
    bool fixedSuperGetDisplayInfo(const UInt32 fbIndex, const bool isCRTEnabled, const bool ignoreCRTOffsetCheck,
        IOFramebuffer *const fb, FramebufferInfo *const fbInfo);
    static bool getDisplayInfo(AMDRadeonX5000_AMDGFX9DCNDisplay *self, UInt32 fbIndex, bool isCRTEnabled,
        bool ignoreCRTOffsetCheck, IOFramebuffer *fb, FramebufferInfo *framebufferInfo);
    static UInt64 getCurrentDisplayOffset(AMDRadeonX5000_AMDGFX9DCNDisplay *self, UInt32 fbIndex);
    static void setCurrentDisplayOffset(AMDRadeonX5000_AMDGFX9DCNDisplay *self, UInt32 fbIndex, UInt64 value);
    static UInt32 writeWaitForVLine(AMDRadeonX5000_AMDGFX9DCNDisplay *self, UInt32 *, UInt32, SInt32 &, SInt32 &, bool,
        bool);
    static void setFlipControlRegister(AMDRadeonX5000_AMDGFX9DCNDisplay *self, UInt32 fbIndex,
        AMDSwapInterval swapInterval);
    static bool init(AMDRadeonX5000_AMDGFX9DCNDisplay *self, void *hwInterface, void *fbParams);
    static ATIPixelMode getPixelMode(AMDRadeonX5000_AMDGFX9DCNDisplay *self, CRTHWDepth depth, CRTHWFormat format);
    static ATIFormat getPixelFormat(AMDRadeonX5000_AMDGFX9DCNDisplay *self, ATIPixelMode pixelMode);
    void fillFlipTilingParameters(AMDFlipParam *flipParam, UInt32 swizzleMode);
    static bool writeFlipParameters(AMDRadeonX5000_AMDGFX9DCNDisplay *self, AMDPipeFlip *pFlip, UInt32 fbIndex,
        UInt64 grphOffsetLeft, UInt64 grphOffsetRight, AMDSwapInterval swapInterval, UInt32 grphPitch,
        const AMDTilingInfo *pTileInfo, ATIFormat atiFormat, UInt32 pixelBytes, AMDHWRotationAngle *hwRotate,
        AMDPipeFlip *pSavedFlip);
    static void getDisplayModeViewportSpecificInfo(AMDRadeonX5000_AMDGFX9DCNDisplay *self, UInt32 fbIndex,
        UInt32 *viewportYStart, UInt32 *viewportHeight);
    static UInt32 writeFlipControlRegisters(AMDRadeonX5000_AMDGFX9DCNDisplay *self, UInt32 fbIndex, UInt32 *buffer,
        AMDSwapInterval swapInterval, UInt64 offsetLeft, UInt64 offsetRight);
    static bool isDisplayControlEnabled(AMDRadeonX5000_AMDGFX9DCNDisplay *self, UInt32 fbIndex);
    static bool isDisplayInterlaceEnabled(AMDRadeonX5000_AMDGFX9DCNDisplay *self, UInt32 fbIndex);
    static bool isFlipPending(AMDRadeonX5000_AMDGFX9DCNDisplay *self, UInt32 fbIndex);
    static AMDFlipOption getFlipOption();

    struct AMDDCNDisplayRegisterOffsets {
        bool isValid;
        UInt32 hubpretControl;
        UInt32 hubpSurfaceConfig;
        UInt32 hubpAddrConfig;
        UInt32 hubpTilingConfig;
        UInt32 hubpPriViewportStart;
        UInt32 hubpPriViewportDimension;
        UInt32 hubpreqSurfacePitch;
        UInt32 hubpreqPrimarySurfaceAddress;
        UInt32 hubpreqPrimarySurfaceAddressHigh;
        UInt32 hubpreqFlipControl;
        UInt32 hubpreqSurfaceEarliestInuse;
        UInt32 hubpreqSurfaceEarliestInuseHigh;
        UInt32 otgControl;
        UInt32 otgInterlaceControl;
    };

    struct AMDDCNDisplayRegisterShiftsMasks {
        bool isValid;
        UInt32 viewportYStartMask;
        UInt8 viewportYStartShift;
        UInt32 viewportHeightMask;
        UInt8 viewportHeightShift;
        UInt32 primarySurfaceHi;
        UInt32 otgEnable;
        UInt32 otgInterlaceEnable;
    };

    struct AMDDCNDisplaySavedState {
        UInt32 hubpreqflipControl;
        UInt32 hubpSurfaceConfig;
        UInt32 hubpControl;
        UInt32 hubpAddrConfig;
        UInt32 hubpTilingConfig;
        AMDHWRotationAngle hwRotation;
        AMDFlipParam flipParam;
    };

    struct Constants {
        void (*setVrrTimestampInfo)(AMDRadeonX5000_AMDGFX9DCNDisplay &self, const UInt64 vTotalMin,
            const UInt64 vTotalMax, const UInt64 horizontalLineTime) {nullptr};
        void (*calcAndSetVrrTimestampInfo)(AMDRadeonX5000_AMDGFX9DCNDisplay &self, FramebufferInfo *const fbInfo,
            IOTimingInformation &timingInfo) {nullptr};

        Constants() {
            // Catalina doesn't have VRR.
            if (currentKernelVersion() < MACOS_11) {
                this->calcAndSetVrrTimestampInfo = AMDRadeonX5000_AMDGFX9DCNDisplay::calcAndSetVrrTimestampInfoDummy;
                return;
            }

            this->calcAndSetVrrTimestampInfo = AMDRadeonX5000_AMDGFX9DCNDisplay::calcAndSetVrrTimestampInfo;
            this->setVrrTimestampInfo = currentKernelVersion() >= MACOS_13 ?
                                            AMDRadeonX5000_AMDGFX9DCNDisplay::setVrrTimestampInfoVentura :
                                            AMDRadeonX5000_AMDGFX9DCNDisplay::setVrrTimestampInfo;
        }
    };

    static Constants constants;

    static void setVrrTimestampInfoVentura(AMDRadeonX5000_AMDGFX9DCNDisplay &self, const UInt64 vTotalMin,
        const UInt64 vTotalMax, const UInt64 horizontalLineTime);
    static void setVrrTimestampInfo(AMDRadeonX5000_AMDGFX9DCNDisplay &self, const UInt64 vTotalMin,
        const UInt64 vTotalMax, const UInt64 horizontalLineTime);
    static void calcAndSetVrrTimestampInfo(AMDRadeonX5000_AMDGFX9DCNDisplay &self, FramebufferInfo *const fbInfo,
        IOTimingInformation &timingInfo);
    static void calcAndSetVrrTimestampInfoDummy(AMDRadeonX5000_AMDGFX9DCNDisplay &self, FramebufferInfo *const fbInfo,
        IOTimingInformation &timingInfo);

    private:
    // 10.15
    static constexpr UInt32 INIT_VT_INDEX_MAC10_15 = 0x50;    // 0x278

    // 10.15+
    static constexpr UInt32 INITIALIZE_REGISTERS_VT_INDEX = 0x23;                       // 0x118
    static constexpr UInt32 RESTORE_REGISTERS_VT_INDEX = 0x24;                          // 0x120
    static constexpr UInt32 GET_DISPLAY_INFO_VT_INDEX = 0x26;                           // 0x130
    static constexpr UInt32 GET_CURRENT_DISPLAY_OFFSET_VT_INDEX = 0x42;                 // 0x210, <=12.0
    static constexpr UInt32 SET_CURRENT_DISPLAY_OFFSET_VT_INDEX = 0x43;                 // 0x218, <=12.0
    static constexpr UInt32 WRITE_WAIT_FOR_VLINE_VT_INDEX = 0x48;                       // 0x240
    static constexpr UInt32 SET_FLIP_CONTROL_REGISTER_VT_INDEX = 0x49;                  // 0x248, <=12.0
    static constexpr UInt32 GET_PIXEL_MODE_VT_INDEX = 0x52;                             // 0x290
    static constexpr UInt32 GET_PIXEL_FORMAT_VT_INDEX = 0x53;                           // 0x298
    static constexpr UInt32 WRITE_FLIP_PARAMETERS_VT_INDEX = 0x56;                      // 0x2B0
    static constexpr UInt32 GET_DISPLAY_MODE_VIEWPORT_SPECIFIC_INFO_VT_INDEX = 0x57;    // 0x2B8
    static constexpr UInt32 WRITE_FLIP_CONTROL_REGISTERS_VT_INDEX = 0x58;               // 0x2C0, <=12.0
    static constexpr UInt32 IS_DISPLAY_CONTROL_ENABLED_VT_INDEX = 0x5A;                 // 0x2D0
    static constexpr UInt32 IS_DISPLAY_INTERLACE_ENABLED_VT_INDEX = 0x5B;               // 0x2D8
    static constexpr UInt32 IS_FLIP_PENDING_VT_INDEX = 0x5C;                            // 0x2E0, <=12.0
    static constexpr UInt32 GET_FLIP_OPTION_VT_INDEX = 0x5D;                            // 0x2E8, >10.15

    // 11.0..=12.0
    static constexpr UInt32 INIT_VT_INDEX = 0x4F;    // 0x278

    // 13.0
    static constexpr UInt32 WRITE_WAIT_FOR_VLINE_VT_INDEX_MAC13 = 0x39;                       // 0x1C8
    static constexpr UInt32 INIT_VT_INDEX_MAC13 = 0x3F;                                       // 0x1F8
    static constexpr UInt32 GET_PIXEL_MODE_VT_INDEX_MAC13 = 0x42;                             // 0x210
    static constexpr UInt32 GET_PIXEL_FORMAT_VT_INDEX_MAC13 = 0x43;                           // 0x218
    static constexpr UInt32 WRITE_FLIP_PARAMETERS_VT_INDEX_MAC13 = 0x46;                      // 0x230
    static constexpr UInt32 GET_DISPLAY_MODE_VIEWPORT_SPECIFIC_INFO_VT_INDEX_MAC13 = 0x47;    // 0x238
    static constexpr UInt32 IS_DISPLAY_CONTROL_ENABLED_VT_INDEX_MAC13 = 0x49;                 // 0x248
    static constexpr UInt32 IS_DISPLAY_INTERLACE_ENABLED_VT_INDEX_MAC13 = 0x4A;               // 0x250
    static constexpr UInt32 GET_FLIP_OPTION_VT_INDEX_MAC13 = 0x4B;                            // 0x258

    protected:
    struct Expansion {
        AMDDCNDisplayRegisterOffsets regOffs[MAX_SUPPORTED_DISPLAYS];
        AMDDCNDisplayRegisterShiftsMasks regShiftsMasks;
        AMDDCNDisplaySavedState savedState[MAX_SUPPORTED_DISPLAYS];
        UInt64 lastSubmitFlipOffset;
    };

    PWDeclareAbstractRuntimeMCWithExpansion(AMDRadeonX5000_AMDGFX9DCNDisplay, Expansion);

    static void populateVFT(RuntimeVFT<vftCount, 1> &vft);

    public:
    static void registerMC(const char *kext, KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);
};
