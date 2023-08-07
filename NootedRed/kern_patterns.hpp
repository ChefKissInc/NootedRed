//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_util.hpp>

/**
 * `__ZL15deviceTypeTable`
 * AMDRadeonX5000HWLibs.kext
 */
static const uint8_t kDeviceTypeTablePattern[] = {0x60, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 0x68, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x68, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x64, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/**
 * `__ZN11AMDFirmware14createFirmwareEPhjjPKc`
 * AMDRadeonX5000HWLibs.kext
 */
static const uint8_t kCreateFirmwarePattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54, 0x53, 0x49,
    0x89, 0xCE, 0x41, 0x89, 0xD7, 0x41, 0x89, 0xF4, 0x48, 0x89, 0xFB, 0xBF, 0x20, 0x00, 0x00, 0x00, 0xE8};

/**
 * `__ZN20AMDFirmwareDirectory11putFirmwareE16_AMD_DEVICE_TYPEP11AMDFirmware`
 * AMDRadeonX5000HWLibs.kext
 */
static const uint8_t kPutFirmwarePattern[] = {0x55, 0x48, 0x89, 0xE5, 0x83, 0xFE, 0x08, 0x7F};

/**
 * `__ZL20CAIL_ASIC_CAPS_TABLE`
 * AMDRadeonX5000HWLibs.kext
 */
static const uint8_t kCailAsicCapsTableHWLibsPattern[] = {0x6E, 0x00, 0x00, 0x00, 0x98, 0x67, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

/**
 * `_smu_get_fw_constants`
 * AMDRadeonX5000HWLibs.kext
 */
static const uint8_t kSmuGetFwConstantsPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54, 0x53,
    0x48, 0x83, 0xEC, 0x10, 0x31, 0xDB, 0x48, 0x89, 0x5D, 0xD8, 0x48, 0x89, 0x5D, 0xD0, 0xF6, 0x87, 0x00, 0x02, 0x00,
    0x00, 0x01};
static const uint8_t kSmuGetFwConstantsMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF,
    0xFF};

/**
 * `_smu_9_0_1_check_fw_status`
 * AMDRadeonX5000HWLibs.kext
 */
static const uint8_t kSmu901CheckFwStatusPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x80, 0xBF, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x74, 0x00, 0x5D, 0xE9, 0x00, 0x00, 0x00, 0x00};
static const uint8_t kSmu901CheckFwStatusMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

/**
 * `_smu_9_0_1_unload_smu`
 * AMDRadeonX5000HWLibs.kext
 */
static const uint8_t kSmu901UnloadSmuPattern[] = {0x55, 0x48, 0x89, 0xE5, 0xBE, 0x40, 0x00, 0x00, 0x00, 0x31, 0xD2,
    0x5D, 0xE9, 0x00, 0x00, 0x00, 0x00};
static const uint8_t kSmu901UnloadSmuMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0x00, 0x00, 0x00};

/**
 * `_psp_cmd_km_submit`
 * AMDRadeonX5000HWLibs.kext
 */
static const uint8_t kPspCmdKmSubmitPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
    0x53, 0x50, 0x49, 0x89, 0xCD, 0x49, 0x89, 0xD7, 0x49, 0x89, 0xF4, 0x48, 0x89, 0xFB, 0x48, 0x8D, 0x75, 0xD0, 0xC7,
    0x06, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00};
static const uint8_t kPspCmdKmSubmitMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

/**
 * `_update_sdma_power_gating`
 * AMDRadeonX5000HWLibs.kext
 */
static const uint8_t kUpdateSdmaPowerGatingPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x53, 0x50,
    0x41, 0x89, 0xF6, 0x48, 0x89, 0xFB, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x41, 0x89, 0xC7, 0x48, 0x89, 0xDF, 0xE8, 0x00,
    0x00, 0x00, 0x00, 0x41, 0xF6, 0xC7, 0x02, 0x75, 0x00};
static const uint8_t kUpdateSdmaPowerGatingMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};

/**
 * `_CAILAsicCapsInitTable`
 * AMDRadeonX5000HWLibs.kext
 */
static const uint8_t kCAILAsicCapsInitTablePattern[] = {0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x98, 0x67,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

/**
 * `_DeviceCapabilityTbl`
 * AMDRadeonX5000HWLibs.kext
 */
static const uint8_t kDeviceCapabilityTblPattern[] = {0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xCA, 0xAD, 0xDE, 0x00, 0x00,
    0x00, 0x00, 0xFE, 0xCA, 0xAD, 0xDE, 0x00, 0x00, 0x00, 0x00};

/**
 * `__ZZN37AMDRadeonX5000_AMDGraphicsAccelerator19createAccelChannelsEbE12channelTypes`
 * AMDRadeonX5000.kext
 */
static const uint8_t kChannelTypesPattern[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};

/**
 * `__ZN4Addr2V27Gfx9Lib20HwlConvertChipFamilyEjj`
 * AMDRadeonX5000.kext
 */
static const uint8_t kHwlConvertChipFamilyPattern[] = {0x81, 0xFE, 0x8D, 0x00, 0x00, 0x00, 0x0F};

/**
 * `__ZL20CAIL_ASIC_CAPS_TABLE`
 * AMDRadeonX6000Framebuffer.kext
 */
static const uint8_t kCailAsicCapsTablePattern[] = {0x6E, 0x00, 0x00, 0x00, 0x98, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

/**
 * `_dce_driver_set_backlight`
 * AMDRadeonX6000Framebuffer.kext
 */
static const uint8_t kDceDriverSetBacklight[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
    0x53, 0x50, 0x41, 0x89, 0xF7, 0x49, 0x89, 0xFE, 0x31, 0xC0, 0x4C, 0x8D, 0x65, 0xD0, 0x41, 0x89, 0x04, 0x24, 0x4C,
    0x8D, 0x6D, 0xD4, 0x41, 0x89, 0x45, 0x00, 0x48, 0x8B, 0x7F, 0x08, 0x49, 0x8B, 0x46, 0x28, 0x8B, 0x70, 0x10};

/**
 * `__ZNK15AmdAtomVramInfo16populateVramInfoER16AtomFirmwareInfo`
 * AMDRadeonX6000Framebuffer.kext
 */
static const uint8_t kPopulateVramInfoPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x53, 0x48, 0x81,
    0xEC, 0x08, 0x01, 0x00, 0x00, 0x49, 0x89, 0xF6, 0x48, 0x89, 0xFB, 0x4C, 0x8D, 0xBD, 0xE0, 0xFE, 0xFF, 0xFF};

/**
 * `__ZNK22AmdAtomObjectInfo_V1_421getNumberOfConnectorsEv`
 * AMDRadeonX6000Framebuffer.kext
 */
static const uint8_t kGetNumberOfConnectorsPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x4C, 0x8B, 0x47, 0x28, 0x41, 0x0F,
    0xB6, 0x40, 0x06, 0x85, 0xC0, 0x74, 0x00};
static const uint8_t kGetNumberOfConnectorsMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};

/**
 * `_IH_4_0_IVRing_InitHardware`
 * AMDRadeonX6000Framebuffer.kext
 */
static const uint8_t kIH40IVRingInitHardwarePattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55,
    0x41, 0x54, 0x53, 0x50, 0x49, 0x89, 0xF6, 0x49, 0x89, 0xFD, 0x48, 0x8B, 0x87, 0x00, 0x44, 0x00, 0x00};
static const uint8_t kIH40IVRingInitHardwareMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF};

/**
 * `_dce_panel_cntl_hw_init`
 * AMDRadeonX6000Framebuffer.kext
 */
static const uint8_t kDcePanelCntlHwInitPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41,
    0x54, 0x53, 0x50, 0x49, 0x89, 0xFD, 0x4C, 0x8D, 0x45, 0xD4, 0x41, 0xC7, 0x00, 0x00, 0x00, 0x00, 0x00};

/**
 * `_IRQMGR_WriteRegister`
 * AMDRadeonX6000Framebuffer.kext
 */
static const uint8_t kIRQMGRWriteRegisterPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41,
    0x54, 0x53, 0x50, 0x41, 0x89, 0xD6, 0x49, 0x89, 0xF7, 0x48, 0x89, 0xFB, 0x48, 0x8B, 0x87, 0xB0, 0x00, 0x00, 0x00,
    0x48, 0x85, 0xC0};

/**
 * `_dm_logger_write`
 * AMDRadeonX6000Framebuffer.kext
 */
static const uint8_t kDmLoggerWritePattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
    0x53, 0x48, 0x81, 0xEC, 0x88, 0x04, 0x00, 0x00};
