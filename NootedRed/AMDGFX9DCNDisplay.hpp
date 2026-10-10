// DCN Display implementation for GFX9
// Derivative of AMDRadeonX5000 and AMDRadeonX6000 decompilation
//
// Copyright (C) 2025-2026  ChefKiss.
//
// Licensed under the ChefKiss Open Knowledge License version 1.
//
// This work is licensed to you under the terms of the ChefKiss Open Knowledge
// License, either version 1 of the License, or (at your option) any later
// version published by ChefKiss.
//
// This work may not be used to train or develop artificial intelligence,
// and may not be used with artificial intelligence in any other way. See
// the ChefKiss Open Knowledge License for details.
//
// This work is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// ChefKiss Open Knowledge License for more details.
//
// You should have received a copy of the ChefKiss Open Knowledge License
// along with this work.  If not, see https://git.chefkiss.dev/ChefKiss/OpenKnowledge/raw/branch/master/LICENSE-v1.md.

#pragma once
#include <GPUDriversAMD/Accel/HWAlignManager.hpp>
#include <GPUDriversAMD/Accel/HWDisplay.hpp>
#include <GPUDriversAMD/FB/FramebufferInfo.hpp>
#include <PenguinWizardry/RuntimeMC.hpp>

class AMDRadeonX5000_AMDGFX9DCNDisplay : public AMDRadeonX5000_AMDHWDisplay
{
    void initDCNRegOffs();

    static void   initialiseRegisters(AMDRadeonX5000_AMDHWDisplay* _self);
    static void   restoreRegisters(AMDRadeonX5000_AMDHWDisplay* _self);
    static bool   getDisplayInfo(AMDRadeonX5000_AMDHWDisplay* _self, UInt32 fbIndex, bool isCRTEnabled,
                                 bool ignoreCRTOffsetCheck, IOFramebuffer* fb, FramebufferInfo* framebufferInfo);
    static UInt64 getCurrentDisplayOffset(AMDRadeonX5000_AMDHWDisplay* _self, UInt32 fbIndex);
    static void   setCurrentDisplayOffset(AMDRadeonX5000_AMDHWDisplay* _self, UInt32 fbIndex, UInt64 value);
    static UInt32 writeWaitForVLine(AMDRadeonX5000_AMDHWDisplay* _self, UInt32*, UInt32, SInt32&, SInt32&, bool, bool);
    static void   setFlipControlRegister(AMDRadeonX5000_AMDHWDisplay* _self, UInt32 fbIndex,
                                         AMDSwapInterval swapInterval);
    static bool   init(AMDRadeonX5000_AMDHWDisplay* _self, void* hwInterface, void* fbParams);
    static ATIPixelMode getPixelMode(AMDRadeonX5000_AMDHWDisplay* _self, CRTHWDepth depth, CRTHWFormat format);
    static ATIFormat    getPixelFormat(AMDRadeonX5000_AMDHWDisplay* _self, ATIPixelMode pixelMode);
    void                fillFlipTilingParameters(AMDFlipParam* flipParam, UInt32 swizzleMode);
    static bool         writeFlipParameters(AMDRadeonX5000_AMDHWDisplay* _self, AMDPipeFlip* pFlip, UInt32 fbIndex,
                                            UInt64 grphOffsetLeft, UInt64 grphOffsetRight, AMDSwapInterval swapInterval,
                                            UInt32 grphPitch, const AMDTilingInfo* pTileInfo, ATIFormat atiFormat,
                                            UInt32 pixelBytes, AMDHWRotationAngle* hwRotate, AMDPipeFlip* pSavedFlip);
    static void         getDisplayModeViewportSpecificInfo(AMDRadeonX5000_AMDHWDisplay* _self, UInt32 fbIndex,
                                                           UInt32* viewportYStart, UInt32* viewportHeight);
    static UInt32       writeFlipControlRegisters(AMDRadeonX5000_AMDHWDisplay* _self, UInt32 fbIndex, UInt32* buffer,
                                                  AMDSwapInterval swapInterval, UInt64 offsetLeft, UInt64 offsetRight);
    static bool         isDisplayControlEnabled(AMDRadeonX5000_AMDHWDisplay* _self, UInt32 fbIndex);
    static bool         isDisplayInterlaceEnabled(AMDRadeonX5000_AMDHWDisplay* _self, UInt32 fbIndex);
    static bool         isFlipPending(AMDRadeonX5000_AMDHWDisplay* _self, UInt32 fbIndex);
    static AMDFlipOption getFlipOption(AMDRadeonX5000_AMDHWDisplay*);
    static UInt32        getNumberOfSupportedDisplays(AMDRadeonX5000_AMDHWDisplay*);

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

protected:
    static constexpr UInt32 MAX_SUPPORTED_DISPLAYS_RV = 4;

    struct Expansion
    {
        AMDDCNDisplayRegisterOffsets     regOffs[MAX_SUPPORTED_DISPLAYS_RV];
        AMDDCNDisplayRegisterShiftsMasks regShiftsMasks;
        AMDDCNDisplaySavedState          savedState[MAX_SUPPORTED_DISPLAYS_RV];
        UInt64                           lastSubmitFlipOffset;
    };

    PWDeclareAbstractRuntimeMCWithExpansion(AMDRadeonX5000_AMDGFX9DCNDisplay, Expansion)

    using VFT = RuntimeVFT<1>;

    static void populateVFT(VFT& vft);

public:
    static void registerMC(const char* kext, KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size);
};
