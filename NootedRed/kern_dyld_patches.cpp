//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_dyld_patches.hpp"
#include "kern_nred.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>
#include <IOKit/IODeviceTreeSupport.h>

DYLDPatches *DYLDPatches::callback = nullptr;

void DYLDPatches::init() { callback = this; }

void DYLDPatches::processPatcher(KernelPatcher &patcher) {
    if (!(lilu.getRunMode() & LiluAPI::RunningNormal) || !checkKernelArgument("-nredvcn")) { return; }

    auto *entry = IORegistryEntry::fromPath("/", gIODTPlane);
    if (entry) {
        DBGLOG("nred", "Setting hwgva-id to MacPro7,1");
        entry->setProperty("hwgva-id", const_cast<char *>(kHwGvaId), arrsize(kHwGvaId));
        entry->release();
    }

    KernelPatcher::RouteRequest request {"_cs_validate_page", csValidatePage, this->orgCsValidatePage};

    PANIC_COND(!patcher.routeMultipleLong(KernelPatcher::KernelID, &request, 1), "nred",
        "Failed to route kernel symbols");
}

void DYLDPatches::csValidatePage(vnode *vp, memory_object_t pager, memory_object_offset_t page_offset, const void *data,
    int *validated_p, int *tainted_p, int *nx_p) {
    FunctionCast(csValidatePage, callback->orgCsValidatePage)(vp, pager, page_offset, data, validated_p, tainted_p,
        nx_p);

    char path[PATH_MAX];
    int pathlen = PATH_MAX;
    if (UNLIKELY(vn_getpath(vp, path, &pathlen))) { return; }

    if (!UserPatcher::matchSharedCachePath(path)) {
        if (LIKELY(strncmp(path, kCoreLSKDMSEPath, arrsize(kCoreLSKDMSEPath))) ||
            LIKELY(strncmp(path, kCoreLSKDPath, arrsize(kCoreLSKDPath)))) {
            return;
        }
        if (UNLIKELY(KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, kCoreLSKDOriginal,
                kCoreLSKDPatched)))
            DBGLOG("nred", "Patched streaming CPUID to Haswell");
    }

    if (UNLIKELY(KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, kVideoToolboxDRMModelOriginal,
            arrsize(kVideoToolboxDRMModelOriginal), BaseDeviceInfo::get().modelIdentifier, 20)))
        DBGLOG("nred", "Relaxed VideoToolbox DRM model check");

    if (UNLIKELY(KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, kAGVABoardIdOriginal,
            arrsize(kAGVABoardIdOriginal), kAGVABoardIdPatched, arrsize(kAGVABoardIdPatched))))
        DBGLOG("nred", "Applied MacPro7,1 spoof to AppleGVA");

    if (UNLIKELY(KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, kHEVCEncBoardIdOriginal,
            kHEVCEncBoardIdPatched)))
        DBGLOG("nred", "Applied MacPro7,1 spoof to AppleGVAHEVCEncoder");

    if (getKernelVersion() >= KernelVersion::Ventura) {
        if (UNLIKELY(KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE,
                kVAAcceleratorInfoIdentifyVenturaOriginal, arrsize(kVAAcceleratorInfoIdentifyVenturaOriginal),
                kVAAcceleratorInfoIdentifyVenturaOriginalMask, arrsize(kVAAcceleratorInfoIdentifyVenturaOriginalMask),
                kVAAcceleratorInfoIdentifyVenturaPatched, arrsize(kVAAcceleratorInfoIdentifyVenturaPatched),
                kVAAcceleratorInfoIdentifyVenturaPatchedMask, arrsize(kVAAcceleratorInfoIdentifyVenturaPatchedMask), 0,
                0)))
            DBGLOG("nred", "Patched VAAcceleratorInfo::identify");
        if (UNLIKELY(KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE,
                kVAFactoryCreateGraphicsEngineAndBltVenturaOriginal,
                arrsize(kVAFactoryCreateGraphicsEngineAndBltVenturaOriginal),
                kVAFactoryCreateGraphicsEngineAndBltVenturaMask,
                arrsize(kVAFactoryCreateGraphicsEngineAndBltVenturaMask), kVAFactoryCreateGraphicsEnginePatched,
                arrsize(kVAFactoryCreateGraphicsEnginePatched), nullptr, 0, 0, 0)))
            DBGLOG("nred", "Patched VAFactory::createGraphicsEngine/VAFactory::createImageBlt");
        if (UNLIKELY(KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE,
                kVAFactoryCreateVPVenturaOriginal, arrsize(kVAFactoryCreateVPVenturaOriginal),
                kVAFactoryCreateVPVenturaOriginalMask, arrsize(kVAFactoryCreateVPVenturaOriginalMask),
                kVAFactoryCreateVPVenturaPatched, arrsize(kVAFactoryCreateVPVenturaPatched),
                kVAFactoryCreateVPVenturaPatchedMask, arrsize(kVAFactoryCreateVPVenturaPatchedMask), 0, 0)))
            DBGLOG("nred", "Patched VAFactory::create*VP");
    } else {
        if (UNLIKELY(KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE,
                kVAAcceleratorInfoIdentifyOriginal, arrsize(kVAAcceleratorInfoIdentifyOriginal),
                kVAAcceleratorInfoIdentifyOriginalMask, arrsize(kVAAcceleratorInfoIdentifyOriginalMask),
                kVAAcceleratorInfoIdentifyPatched, arrsize(kVAAcceleratorInfoIdentifyPatched),
                kVAAcceleratorInfoIdentifyPatchedMask, arrsize(kVAAcceleratorInfoIdentifyPatchedMask), 0, 0)))
            DBGLOG("nred", "Patched VAAcceleratorInfo::identify");
        if (UNLIKELY(KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE,
                kVAFactoryCreateGraphicsEngineOriginal, arrsize(kVAFactoryCreateGraphicsEngineOriginal),
                kVAFactoryCreateGraphicsEngineMask, arrsize(kVAFactoryCreateGraphicsEngineMask),
                kVAFactoryCreateGraphicsEnginePatched, arrsize(kVAFactoryCreateGraphicsEnginePatched), nullptr, 0, 0,
                0)))
            DBGLOG("nred", "Patched VAFactory::createGraphicsEngine");
        if (UNLIKELY(
                KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE, kVAFactoryCreateVPOriginal,
                    arrsize(kVAFactoryCreateVPOriginal), kVAFactoryCreateVPMask, arrsize(kVAFactoryCreateVPMask),
                    kVAFactoryCreateVPPatched, arrsize(kVAFactoryCreateVPPatched), nullptr, 0, 0, 0)))
            DBGLOG("nred", "Patched VAFactory::create*VP");
        if (UNLIKELY(KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE,
                kVAFactoryCreateImageBltOriginal, arrsize(kVAFactoryCreateImageBltOriginal),
                kVAFactoryCreateImageBltMask, arrsize(kVAFactoryCreateImageBltMask), kVAFactoryCreateImageBltPatched,
                arrsize(kVAFactoryCreateImageBltPatched), nullptr, 0, 0, 0)))
            DBGLOG("nred", "Patched VAFactory::createImageBlt");
    }

    if (UNLIKELY(KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE,
            kVAAddrLibInterfaceInitOriginal, kVAAddrLibInterfaceInitOriginalMask, kVAAddrLibInterfaceInitPatched,
            kVAAddrLibInterfaceInitPatchedMask, 0, 0)))
        DBGLOG("nred", "Patched VAAddrLibInterface::init");

    // ----------------------------------------------
    if (NRed::callback->chipType >= ChipType::Renoir) { return; }    // Everything after is for VCN 1
    // ----------------------------------------------

    if (UNLIKELY(KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, kWriteUvdNoOpOriginal,
            kWriteUvdNoOpPatched)))
        DBGLOG("nred", "Patched Vcn2DecCommand::writeUvdNoOp");

    if (UNLIKELY(KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, kWriteUvdEngineStartOriginal,
            kWriteUvdEngineStartPatched)))
        DBGLOG("nred", "Patched Vcn2DecCommand::writeUvdEngineStart");

    if (UNLIKELY(KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, kWriteUvdGpcomVcpuCmdOriginal,
            kWriteUvdGpcomVcpuCmdPatched)))
        DBGLOG("nred", "Patched Vcn2DecCommand::writeUvdGpcomVcpuCmdOriginal");

    if (UNLIKELY(KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, kWriteUvdGpcomVcpuData0Original,
            kWriteUvdGpcomVcpuData0Patched)))
        DBGLOG("nred", "Patched Vcn2DecCommand::writeUvdGpcomVcpuData0Original");

    if (UNLIKELY(KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, kWriteUvdGpcomVcpuData1Original,
            kWriteUvdGpcomVcpuData1Patched)))
        DBGLOG("nred", "Patched Vcn2DecCommand::writeUvdGpcomVcpuData1Original");

    if (UNLIKELY(KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE, kAddEncodePacketOriginal,
            kAddEncodePacketMask, kAddEncodePacketPatched, kAddEncodePacketMask, 0, 0)))
        DBGLOG("nred", "Patched Vcn2EncCommand::addEncodePacket");

    if (UNLIKELY(
            KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE, kAddSliceHeaderPacketOriginal,
                kAddSliceHeaderPacketMask, kAddSliceHeaderPacketPatched, kAddSliceHeaderPacketMask, 0, 0)))
        DBGLOG("nred", "Patched Vcn2EncCommand::addSliceHeaderPacket");

    if (UNLIKELY(
            KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE, kAddIntraRefreshPacketOriginal,
                kAddIntraRefreshPacketMask, kAddIntraRefreshPacketPatched, kAddIntraRefreshPacketMask, 0, 0)))
        DBGLOG("nred", "Patched Vcn2EncCommand::addIntraRefreshPacket");

    if (UNLIKELY(
            KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE, kAddContextBufferPacketOriginal,
                kAddContextBufferPacketMask, kAddContextBufferPacketPatched, kAddContextBufferPacketMask, 0, 0)))
        DBGLOG("nred", "Patched Vcn2EncCommand::addContextBufferPacket");

    if (UNLIKELY(KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, kAddBitstreamBufferPacketOriginal,
            kAddBitstreamBufferPacketPatched)))
        DBGLOG("nred", "Patched Vcn2EncCommand::addBitstreamBufferPacket");

    if (UNLIKELY(KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, kAddFeedbackBufferPacketOriginal,
            arrsize(kAddFeedbackBufferPacketOriginal), kAddFeedbackBufferPacketPatched,
            arrsize(kAddFeedbackBufferPacketOriginal))))
        DBGLOG("nred", "Patched Vcn2EncCommand::addFeedbackBufferPacket");

    if (UNLIKELY(KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE,
            kAddInputFormatPacketOriginal, arrsize(kAddInputFormatPacketOriginal), kAddFormatPacketMask,
            arrsize(kAddFormatPacketMask), kRetZero, arrsize(kRetZero), nullptr, 0, 0, 0)))
        DBGLOG("nred", "Patched Vcn2EncCommand::addInputFormatPacket");

    if (UNLIKELY(KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE,
            kAddOutputFormatPacketOriginal, arrsize(kAddOutputFormatPacketOriginal), kAddFormatPacketMask,
            arrsize(kAddFormatPacketMask), kRetZero, arrsize(kRetZero), nullptr, 0, 0, 0)))
        DBGLOG("nred", "Patched Vcn2EncCommand::addOutputFormatPacket");
}