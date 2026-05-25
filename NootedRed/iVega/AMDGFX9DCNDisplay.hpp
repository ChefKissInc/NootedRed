// DCN Display implementation for GFX9
// Derivative of AMDRadeonX5000 and AMDRadeonX6000 decompilation
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/Accel/HWAlignManager.hpp>
#include <GPUDriversAMD/Accel/HWDisplay.hpp>
#include <GPUDriversAMD/FB/FramebufferInfo.hpp>
#include <PenguinWizardry/RuntimeMC.hpp>

class AMDRadeonX5000_AMDGFX9DCNDisplay : public AMDRadeonX5000_AMDHWDisplay
{
    void initDCNRegOffs()
    {
        auto vtable = getMember<void**>(this, 0);
        reinterpret_cast<void (*)(const void*)>(vtable[vftCount()])(this);
    }

    static void initialiseRegisters(AMDRadeonX5000_AMDGFX9DCNDisplay* self);
    static void restoreRegisters();
    bool        fixedSuperGetDisplayInfo(const UInt32 fbIndex, const bool isCRTEnabled, const bool ignoreCRTOffsetCheck,
                                         IOFramebuffer* const fb, FramebufferInfo* const fbInfo);
    static bool getDisplayInfo(AMDRadeonX5000_AMDGFX9DCNDisplay* self, UInt32 fbIndex, bool isCRTEnabled,
                               bool ignoreCRTOffsetCheck, IOFramebuffer* fb, FramebufferInfo* framebufferInfo);
    static UInt64 getCurrentDisplayOffset(AMDRadeonX5000_AMDGFX9DCNDisplay* self, UInt32 fbIndex);
    static void   setCurrentDisplayOffset(AMDRadeonX5000_AMDGFX9DCNDisplay* self, UInt32 fbIndex, UInt64 value);
    static UInt32 writeWaitForVLine(AMDRadeonX5000_AMDGFX9DCNDisplay* self, UInt32*, UInt32, SInt32&, SInt32&, bool,
                                    bool);
    static void   setFlipControlRegister(AMDRadeonX5000_AMDGFX9DCNDisplay* self, UInt32 fbIndex,
                                         AMDSwapInterval swapInterval);
    static bool   init(AMDRadeonX5000_AMDGFX9DCNDisplay* self, void* hwInterface, void* fbParams);
    static ATIPixelMode getPixelMode(AMDRadeonX5000_AMDGFX9DCNDisplay* self, CRTHWDepth depth, CRTHWFormat format);
    static ATIFormat    getPixelFormat(AMDRadeonX5000_AMDGFX9DCNDisplay* self, ATIPixelMode pixelMode);
    void                fillFlipTilingParameters(AMDFlipParam* flipParam, UInt32 swizzleMode);
    static bool         writeFlipParameters(AMDRadeonX5000_AMDGFX9DCNDisplay* self, AMDPipeFlip* pFlip, UInt32 fbIndex,
                                            UInt64 grphOffsetLeft, UInt64 grphOffsetRight, AMDSwapInterval swapInterval,
                                            UInt32 grphPitch, const AMDTilingInfo* pTileInfo, ATIFormat atiFormat,
                                            UInt32 pixelBytes, AMDHWRotationAngle* hwRotate, AMDPipeFlip* pSavedFlip);
    static void         getDisplayModeViewportSpecificInfo(AMDRadeonX5000_AMDGFX9DCNDisplay* self, UInt32 fbIndex,
                                                           UInt32* viewportYStart, UInt32* viewportHeight);
    static UInt32 writeFlipControlRegisters(AMDRadeonX5000_AMDGFX9DCNDisplay* self, UInt32 fbIndex, UInt32* buffer,
                                            AMDSwapInterval swapInterval, UInt64 offsetLeft, UInt64 offsetRight);
    static bool   isDisplayControlEnabled(AMDRadeonX5000_AMDGFX9DCNDisplay* self, UInt32 fbIndex);
    static bool   isDisplayInterlaceEnabled(AMDRadeonX5000_AMDGFX9DCNDisplay* self, UInt32 fbIndex);
    static bool   isFlipPending(AMDRadeonX5000_AMDGFX9DCNDisplay* self, UInt32 fbIndex);
    static AMDFlipOption getFlipOption();
    static UInt32        getNumberOfSupportedDisplays();

    struct AMDDCNDisplayRegisterOffsets
    {
        bool   isValid;
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

    struct AMDDCNDisplayRegisterShiftsMasks
    {
        bool   isValid;
        UInt32 viewportYStartMask;
        UInt8  viewportYStartShift;
        UInt32 viewportHeightMask;
        UInt8  viewportHeightShift;
        UInt32 primarySurfaceHi;
        UInt32 otgEnable;
        UInt32 otgInterlaceEnable;
    };

    struct AMDDCNDisplaySavedState
    {
        UInt32             hubpreqflipControl;
        UInt32             hubpSurfaceConfig;
        UInt32             hubpControl;
        UInt32             hubpAddrConfig;
        UInt32             hubpTilingConfig;
        AMDHWRotationAngle hwRotation;
        AMDFlipParam       flipParam;
    };

    using VFT = RuntimeVFT<vftCount, 1>;

    static constexpr UInt32 INVALID_VT_INDEX = 0xFFFFFFFFU;

    struct Constants
    {
        void (*setVrrTimestampInfo)(AMDRadeonX5000_AMDGFX9DCNDisplay& self, const UInt64 vTotalMin,
                                    const UInt64 vTotalMax, const UInt64 horizontalLineTime){nullptr};
        void (*calcAndSetVrrTimestampInfo)(AMDRadeonX5000_AMDGFX9DCNDisplay& self, FramebufferInfo* const fbInfo,
                                           IOTimingInformation& timingInfo){
            AMDRadeonX5000_AMDGFX9DCNDisplay::calcAndSetVrrTimestampInfoDummy};

        // These remain constant, for now
        static constexpr UInt32 initializeRegistersVTIndex = 0x23;
        static constexpr UInt32 restoreRegistersVTIndex    = 0x24;
        static constexpr UInt32 getDisplayInfoVTIndex      = 0x26;

        UInt32 getCurrentDisplayOffsetVTIndex{INVALID_VT_INDEX};
        UInt32 setCurrentDisplayOffsetVTIndex{INVALID_VT_INDEX};
        UInt32 writeWaitForVLineVTIndex{INVALID_VT_INDEX};
        UInt32 setFlipControlRegisterVTIndex{INVALID_VT_INDEX};
        UInt32 initVTIndex{INVALID_VT_INDEX};
        UInt32 getPixelModeVTIndex{INVALID_VT_INDEX};
        UInt32 getPixelFormatVTIndex{INVALID_VT_INDEX};
        UInt32 writeFlipParametersVTIndex{INVALID_VT_INDEX};
        UInt32 getDisplayModeViewportSpecificInfoVTIndex{INVALID_VT_INDEX};
        UInt32 writeFlipControlRegistersVTIndex{INVALID_VT_INDEX};
        UInt32 isDisplayControlEnabledVTIndex{INVALID_VT_INDEX};
        UInt32 isDisplayInterlaceEnabledVTIndex{INVALID_VT_INDEX};
        UInt32 isFlipPendingVTIndex{INVALID_VT_INDEX};
        UInt32 getFlipOptionVTIndex{INVALID_VT_INDEX};
        UInt32 getNumberOfSupportedDisplaysVTIndex{INVALID_VT_INDEX};

        Constants()
        {
            if (currentKernelVersion() <= MACOS_10_14_X) {
                this->getCurrentDisplayOffsetVTIndex            = 0x44;
                this->setCurrentDisplayOffsetVTIndex            = 0x45;
                this->writeWaitForVLineVTIndex                  = 0x4A;
                this->setFlipControlRegisterVTIndex             = 0x4B;
                this->initVTIndex                               = 0x51;
                this->getPixelModeVTIndex                       = 0x53;
                this->getPixelFormatVTIndex                     = 0x54;
                this->writeFlipParametersVTIndex                = 0x57;
                this->getDisplayModeViewportSpecificInfoVTIndex = 0x59;
                this->writeFlipControlRegistersVTIndex          = 0x5A;
                this->isDisplayControlEnabledVTIndex            = 0x5D;
                this->isDisplayInterlaceEnabledVTIndex          = 0x5E;
                this->isFlipPendingVTIndex                      = 0x5F;
                this->getNumberOfSupportedDisplaysVTIndex       = 0x3F;
                return;
            }

            if (currentKernelVersion() >= MACOS_11) {
                this->calcAndSetVrrTimestampInfo = AMDRadeonX5000_AMDGFX9DCNDisplay::calcAndSetVrrTimestampInfo;
                if (currentKernelVersion() >= MACOS_13) {
                    this->setVrrTimestampInfo        = AMDRadeonX5000_AMDGFX9DCNDisplay::setVrrTimestampInfoVentura;
                    this->writeWaitForVLineVTIndex   = 0x39;
                    this->initVTIndex                = 0x3F;
                    this->getPixelModeVTIndex        = 0x42;
                    this->getPixelFormatVTIndex      = 0x43;
                    this->writeFlipParametersVTIndex = 0x46;
                    this->getDisplayModeViewportSpecificInfoVTIndex = 0x47;
                    this->isDisplayControlEnabledVTIndex            = 0x49;
                    this->isDisplayInterlaceEnabledVTIndex          = 0x4A;
                    this->getFlipOptionVTIndex                      = 0x4B;
                    return;
                }

                this->setVrrTimestampInfo  = AMDRadeonX5000_AMDGFX9DCNDisplay::setVrrTimestampInfo;
                this->initVTIndex          = 0x4F;
                this->getFlipOptionVTIndex = 0x5D;
            }
            else {
                this->initVTIndex = 0x50;
            }

            this->getCurrentDisplayOffsetVTIndex            = 0x42;
            this->setCurrentDisplayOffsetVTIndex            = 0x43;
            this->setFlipControlRegisterVTIndex             = 0x49;
            this->writeWaitForVLineVTIndex                  = 0x48;
            this->getPixelModeVTIndex                       = 0x52;
            this->getPixelFormatVTIndex                     = 0x53;
            this->writeFlipParametersVTIndex                = 0x56;
            this->getDisplayModeViewportSpecificInfoVTIndex = 0x57;
            this->writeFlipControlRegistersVTIndex          = 0x58;
            this->isDisplayControlEnabledVTIndex            = 0x5A;
            this->isDisplayInterlaceEnabledVTIndex          = 0x5B;
            this->isFlipPendingVTIndex                      = 0x5C;
        }
    };

    static Constants constants;

    static void setVrrTimestampInfoVentura(AMDRadeonX5000_AMDGFX9DCNDisplay& self, const UInt64 vTotalMin,
                                           const UInt64 vTotalMax, const UInt64 horizontalLineTime);
    static void setVrrTimestampInfo(AMDRadeonX5000_AMDGFX9DCNDisplay& self, const UInt64 vTotalMin,
                                    const UInt64 vTotalMax, const UInt64 horizontalLineTime);
    static void calcAndSetVrrTimestampInfo(AMDRadeonX5000_AMDGFX9DCNDisplay& self, FramebufferInfo* const fbInfo,
                                           IOTimingInformation& timingInfo);
    static void calcAndSetVrrTimestampInfoDummy(AMDRadeonX5000_AMDGFX9DCNDisplay& self, FramebufferInfo* const fbInfo,
                                                IOTimingInformation& timingInfo);

protected:
    static constexpr UInt32 MAX_SUPPORTED_DISPLAYS_RV = 4;

    struct Expansion
    {
        AMDDCNDisplayRegisterOffsets     regOffs[MAX_SUPPORTED_DISPLAYS_RV];
        AMDDCNDisplayRegisterShiftsMasks regShiftsMasks;
        AMDDCNDisplaySavedState          savedState[MAX_SUPPORTED_DISPLAYS_RV];
        UInt64                           lastSubmitFlipOffset;
    };

    PWDeclareAbstractRuntimeMCWithExpansion(AMDRadeonX5000_AMDGFX9DCNDisplay, Expansion);

    static void populateVFT(VFT& vft);

public:
    static void registerMC(const char* kext, KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size);
};
