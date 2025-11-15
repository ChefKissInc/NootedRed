// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <GPUDriversAMD/RavenIPOffset.hpp>
#include <iVega/AMDGFX9DCN2Display.hpp>
#include <iVega/Regs/DCN2.hpp>

PWDefineRuntimeMC(AMDRadeonX5000_AMDGFX9DCN2Display, Constructor);

RuntimeVFT<AMDRadeonX5000_AMDHWDisplay::vftCount, 1> AMDRadeonX5000_AMDGFX9DCN2Display::vft {};

void AMDRadeonX5000_AMDGFX9DCN2Display::Constructor(AMDRadeonX5000_AMDGFX9DCN2Display *const self,
    const OSMetaClass *const metaClass) {
    assert(AMDRadeonX5000_AMDHWDisplay::constructor() != 0);
    FunctionCast(Constructor, AMDRadeonX5000_AMDHWDisplay::constructor())(self, metaClass);
    vft.replaceVFT(self);
    gRTMetaClass.instanceConstructed();
}

void AMDRadeonX5000_AMDGFX9DCN2Display::resolve(const char *const kext) {
    AMDRadeonX5000_AMDGFX9DCNDisplay::populateVFT(vft);
    PWPopulateRuntimeMCGetMetaClassVFTEntry();
    vft.getExpanded<decltype(initDCNRegOffs)>(0) = initDCNRegOffs;

    PenguinWizardry::RuntimeMCManager::singleton().registerMC(gRTMetaClass, kext,
        AMDRadeonX5000_AMDGFX9DCNDisplay::gRTMetaClass);
}

void AMDRadeonX5000_AMDGFX9DCN2Display::initDCNRegOffs(AMDRadeonX5000_AMDGFX9DCN2Display *const self) {
    auto &expansion = self->getExpansion();
    for (UInt32 i = 0; i < 6; i += 1) {
        const UInt32 hubpRegStride = HUBP_REG_STRIDE * i;
        const UInt32 otgRegStride = OTG_REG_STRIDE * i;
        auto &regOffs = expansion.regOffs[i];
        regOffs.isValid = true;
        regOffs.hubpretControl = DCN_BASE_2 + HUBPRET_CONTROL + hubpRegStride;
        regOffs.hubpSurfaceConfig = DCN_BASE_2 + HUBP_SURFACE_CONFIG + hubpRegStride;
        regOffs.hubpAddrConfig = DCN_BASE_2 + HUBP_ADDR_CONFIG + hubpRegStride;
        regOffs.hubpTilingConfig = DCN_BASE_2 + HUBP_TILING_CONFIG + hubpRegStride;
        regOffs.hubpPriViewportStart = DCN_BASE_2 + HUBP_PRI_VIEWPORT_START + hubpRegStride;
        regOffs.hubpPriViewportDimension = DCN_BASE_2 + HUBP_PRI_VIEWPORT_DIMENSION + hubpRegStride;
        regOffs.hubpreqSurfacePitch = DCN_BASE_2 + HUBPREQ_SURFACE_PITCH + hubpRegStride;
        regOffs.hubpreqPrimarySurfaceAddress = DCN_BASE_2 + HUBPREQ_PRIMARY_SURFACE_ADDRESS + hubpRegStride;
        regOffs.hubpreqPrimarySurfaceAddressHigh = DCN_BASE_2 + HUBPREQ_PRIMARY_SURFACE_ADDRESS_HIGH + hubpRegStride;
        regOffs.hubpreqFlipControl = DCN_BASE_2 + HUBPREQ_FLIP_CONTROL + hubpRegStride;
        regOffs.hubpreqSurfaceEarliestInuse = DCN_BASE_2 + HUBPREQ_SURFACE_EARLIEST_INUSE + hubpRegStride;
        regOffs.hubpreqSurfaceEarliestInuseHigh = DCN_BASE_2 + HUBPREQ_SURFACE_EARLIEST_INUSE_HIGH + hubpRegStride;
        regOffs.otgControl = DCN_BASE_2 + OTG_CONTROL + otgRegStride;
        regOffs.otgInterlaceControl = DCN_BASE_2 + OTG_INTERLACE_CONTROL + otgRegStride;
    }

    expansion.regShiftsMasks.viewportYStartMask = 0x3FFF0000;
    expansion.regShiftsMasks.viewportYStartShift = 16;
    expansion.regShiftsMasks.viewportHeightMask = 0x3FFF0000;
    expansion.regShiftsMasks.viewportHeightShift = 16;
    expansion.regShiftsMasks.primarySurfaceHi = 0xFFFF;
    expansion.regShiftsMasks.otgEnable = 1;
    expansion.regShiftsMasks.otgInterlaceEnable = 1;
    expansion.regShiftsMasks.isValid = true;
}
