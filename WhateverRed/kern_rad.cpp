//
//  kern_rad.cpp
//  WhateverRed
//
//  Copyright © 2017 vit9696. All rights reserved.
//  Copyright © 2022 VisualDevelopment. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_iokit.hpp>
#include <Headers/kern_devinfo.hpp>
#include <IOKit/IOService.h>

#include <Availability.h>
#include <IOKit/IOPlatformExpert.h>

#include "kern_rad.hpp"
#include "kern_fw.hpp"

#define WRAP_SIMPLE(ty, func, fmt)											\
	ty RAD::wrap##func(void* that)											\
	{																		\
		SYSLOG("rad", #func " called!");									\
		auto ret = FunctionCast(wrap##func, callbackRAD->org##func)(that);	\
		SYSLOG("rad", #func " returned " fmt, ret);							\
		return ret;															\
	}

static const char *pathAMD10000Controller[] = {"/System/Library/Extensions/AMD10000Controller.kext/Contents/MacOS/AMD10000Controller"};
static const char *pathFramebuffer[] = {"/System/Library/Extensions/AMDFramebuffer.kext/Contents/MacOS/AMDFramebuffer"};
static const char *pathSupport[] = {"/System/Library/Extensions/AMDSupport.kext/Contents/MacOS/AMDSupport"};
static const char *pathRadeonX5000[] = {"/System/Library/Extensions/AMDRadeonX5000.kext/Contents/MacOS/AMDRadeonX5000"};
static const char *pathRadeonX5000HWLibs[] = {
	"/System/Library/Extensions/AMDRadeonX5000HWServices.kext/Contents/PlugIns/AMDRadeonX5000HWLibs.kext/Contents/MacOS/AMDRadeonX5000HWLibs",
};

static KernelPatcher::KextInfo kextAMD10000Controller{"com.apple.kext.AMD10000Controller", pathAMD10000Controller, 1, {}, {}, KernelPatcher::KextInfo::Unloaded};
static KernelPatcher::KextInfo kextRadeonFramebuffer{"com.apple.kext.AMDFramebuffer", pathFramebuffer, 1, {}, {}, KernelPatcher::KextInfo::Unloaded};
static KernelPatcher::KextInfo kextRadeonSupport{"com.apple.kext.AMDSupport", pathSupport, 1, {}, {}, KernelPatcher::KextInfo::Unloaded};
static KernelPatcher::KextInfo kextRadeonX5000HWLibs{"com.apple.kext.AMDRadeonX5000HWLibs", pathRadeonX5000HWLibs, 1, {}, {}, KernelPatcher::KextInfo::Unloaded};

static KernelPatcher::KextInfo kextRadeonHardware[] = {
	{"com.apple.kext.AMDRadeonX5000", pathRadeonX5000, arrsize(pathRadeonX5000), {}, {}, KernelPatcher::KextInfo::Unloaded},
};

/**
 *  Power-gating flags
 *  Each symbol corresponds to a bit provided in a radpg argument mask
 */
static const char *powerGatingFlags[] = {
	"CAIL_DisableDrmdmaPowerGating",
	"CAIL_DisableGfxCGPowerGating",
	"CAIL_DisableUVDPowerGating",
	"CAIL_DisableVCEPowerGating",
	"CAIL_DisableDynamicGfxMGPowerGating",
	"CAIL_DisableGmcPowerGating",
	"CAIL_DisableAcpPowerGating",
	"CAIL_DisableSAMUPowerGating",
};

RAD *RAD::callbackRAD;

void RAD::init()
{
	callbackRAD = this;
	
	currentPropProvider.init();
	
	force24BppMode = checkKernelArgument("-rad24");
	
	if (force24BppMode) lilu.onKextLoadForce(&kextRadeonFramebuffer);
	
	dviSingleLink = checkKernelArgument("-raddvi");
	fixConfigName = checkKernelArgument("-radcfg");
	forceVesaMode = checkKernelArgument("-radvesa");
	forceCodecInfo = checkKernelArgument("-radcodec");
	
	lilu.onKextLoadForce(&kextAMD10000Controller);
	lilu.onKextLoadForce(&kextRadeonSupport);
	lilu.onKextLoadForce(&kextRadeonX5000HWLibs);
	
	initHardwareKextMods();
	
		// FIXME: autodetect?
	uint32_t powerGatingMask = 0;
	PE_parse_boot_argn("radpg", &powerGatingMask, sizeof(powerGatingMask));
	for (size_t i = 0; i < arrsize(powerGatingFlags); i++)
	{
		if (!(powerGatingMask & (1 << i)))
		{
			DBGLOG("rad", "not enabling %s", powerGatingFlags[i]);
			powerGatingFlags[i] = nullptr;
		}
		else
		{
			DBGLOG("rad", "enabling %s", powerGatingFlags[i]);
		}
	}
}

void RAD::deinit()
{
}

void RAD::processKernel(KernelPatcher &patcher, DeviceInfo *info)
{
	for (size_t i = 0; i < info->videoExternal.size(); i++)
	{
		if (info->videoExternal[i].vendor == WIOKit::VendorID::ATIAMD)
		{
			if (info->videoExternal[i].video->getProperty("enable-gva-support"))
				enableGvaSupport = true;
			
			auto smufw = OSDynamicCast(OSData, info->videoExternal[i].video->getProperty("Force_Load_FalconSMUFW"));
			if (smufw && smufw->getLength() == 1)
			{
				info->videoExternal[i].video->setProperty("Force_Load_FalconSMUFW",
														  *static_cast<const uint8_t *>(smufw->getBytesNoCopy()) ? kOSBooleanTrue : kOSBooleanFalse);
			}
		}
	}
	
	int gva;
	if (PE_parse_boot_argn("radgva", &gva, sizeof(gva)))
		enableGvaSupport = gva != 0;
	
	KernelPatcher::RouteRequest requests[] = {
		KernelPatcher::RouteRequest("__ZN15IORegistryEntry11setPropertyEPKcPvj", wrapSetProperty, orgSetProperty),
		KernelPatcher::RouteRequest("__ZNK15IORegistryEntry11getPropertyEPKc", wrapGetProperty, orgGetProperty),
	};
	patcher.routeMultiple(KernelPatcher::KernelID, requests);
}

IOReturn RAD::wrapProjectByPartNumber([[maybe_unused]] IOService* that, [[maybe_unused]] uint64_t partNumber) {
	return kIOReturnNotFound;
}

WRAP_SIMPLE(uint64_t, InitializeProjectDependentResources, "0x%llx")
WRAP_SIMPLE(uint64_t, HwInitializeFbMemSize, "0x%llx")
WRAP_SIMPLE(uint64_t, HwInitializeFbBase, "0x%llx")

uint64_t RAD::wrapInitWithController(void *that, void *controller)
{
	SYSLOG("rad", "initWithController called!");
	auto ret = FunctionCast(wrapInitWithController, callbackRAD->orgInitWithController)(that, controller);
	SYSLOG("rad", "initWithController returned %llx", ret);
	return ret;
}

uint64_t RAD::createVramInfo([[maybe_unused]] void *helper, [[maybe_unused]] uint32_t offset)
{
	SYSLOG("rad", "createVramInfo called! Returning fake value");
	return 1;
}

void RAD::wrapAmdTtlServicesConstructor(IOService *that, IOPCIDevice *provider)
{
	SYSLOG("rad", "patching device type table");
	MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock);
	*(uint32_t *)callbackRAD->orgDeviceTypeTable = provider->extendedConfigRead16(kIOPCIConfigDeviceID);
	*((uint32_t *)callbackRAD->orgDeviceTypeTable + 1) = 6;
	MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
	
	SYSLOG("rad", "calling original AmdTtlServices constructor");
	FunctionCast(wrapAmdTtlServicesConstructor, callbackRAD->orgAmdTtlServicesConstructor)(that, provider);
}

uint64_t RAD::wrapTtlDevSetSmuFwVersion(void *tlsInstance, uint32_t *b)
{
	SYSLOG("rad", "_ttlDevSetSmuFwVersion called!");
	SYSLOG("rad", "_ttlDevSetSmuFwVersion: tlsInstance = %p param2 = %p", tlsInstance, b);
	SYSLOG("rad", "_ttlDevSetSmuFwVersion: param2 0:0x%x 1:0x%x 2:0x%x 3:0x%x 4:0x%x 5:0x%x 6:0x%x 7:0x%x", b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
	auto ret = FunctionCast(wrapTtlDevSetSmuFwVersion, callbackRAD->orgTtlDevSetSmuFwVersion)(tlsInstance, b);
	SYSLOG("rad", "_ttlDevSetSmuFwVersion returned 0x%x", ret);
	return ret;
}

uint64_t RAD::wrapIpiSetFwEntry(void *tlsInstance, void *b)
{
	SYSLOG("rad", "_IpiSetFwEntry called!");
	SYSLOG("rad", "_IpiSetFwEntry: tlsInstance = %p param2 = %p", tlsInstance, b);
	auto ret = FunctionCast(wrapIpiSetFwEntry, callbackRAD->orgIpiSetFwEntry)(tlsInstance, b);
	SYSLOG("rad", "_IpiSetFwEntry returned 0x%x", ret);
	return ret;
}

uint64_t RAD::wrapIpiSmuSwInit(void *tlsInstance)
{
	SYSLOG("rad", "_ipi_smu_sw_init called!");
	SYSLOG("rad", "_ipi_smu_sw_init: tlsInstance = %p", tlsInstance);
	auto ret = FunctionCast(wrapIpiSmuSwInit, callbackRAD->orgIpiSmuSwInit)(tlsInstance);
	SYSLOG("rad", "_ipi_smu_sw_init returned 0x%x", ret);
	return ret;
}

uint64_t RAD::wrapSmuSwInit(void *input, uint64_t *output)
{
	SYSLOG("rad", "_smu_sw_init called!");
	SYSLOG("rad", "_smu_sw_init: input = %p output = %p", input, output);
	auto ret = FunctionCast(wrapSmuSwInit, callbackRAD->orgSmuSwInit)(input, output);
	SYSLOG("rad", "_smu_sw_init: output 0:0x%llx 1:0x%llx", output[0], output[1]);
	SYSLOG("rad", "_smu_sw_init returned 0x%x", ret);
	return ret;
}

uint32_t RAD::wrapSmuInternalSwInit(uint64_t param1, uint64_t param2, void *param3)
{
	SYSLOG("rad", "_smu_internal_sw_init called!");
	SYSLOG("rad", "_smu_internal_sw_init: param1 = 0x%llx param2 = 0x%llx param3 = %p", param1, param2, param3);
	auto ret = FunctionCast(wrapSmuInternalSwInit, callbackRAD->orgSmuInternalSwInit)(param1, param2, param3);
	SYSLOG("rad", "_smu_internal_sw_init returned 0x%x", ret);
	return ret;
}

uint64_t RAD::wrapSmuGetHwVersion(uint64_t param1, uint32_t param2)
{
	SYSLOG("rad", "_smu_get_hw_version called!");
	SYSLOG("rad", "_smu_get_hw_version: param1 = 0x%llx param2 = 0x%x", param1, param2);
	auto ret = FunctionCast(wrapSmuGetHwVersion, callbackRAD->orgSmuGetHwVersion)(param1, param2);
	SYSLOG("rad", "_smu_get_hw_version returned 0x%llx", ret);
	return ret == 0xC || ret == 0xB ? 0x3 : ret;
}

uint64_t RAD::wrapPspSwInit(uint32_t *param1, uint32_t *param2)
{
	SYSLOG("rad", "_psp_sw_init called!");
	SYSLOG("rad", "_psp_sw_init: param1 = %p param2 = %p", param1, param2);
	SYSLOG("rad", "_psp_sw_init: param1: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", param1[0], param1[1], param1[2], param1[3], param1[4], param1[5]);
	switch (param1[3]) {
		case 0xA:
			[[fallthrough]];
		case 0xB:
			[[fallthrough]];
		case 0xC:
			SYSLOG("rad", "Spoofing PSP version 10/11/12 to 11");
			param1[3] = 0xB;
			param1[4] = 0x0;
			param1[5] = 0x0;
			break;
		default:
			break;
	}
	auto ret = FunctionCast(wrapPspSwInit, callbackRAD->orgPspSwInit)(param1, param2);
	SYSLOG("rad", "_psp_sw_init returned 0x%llx", ret);
	return ret;
}

uint32_t RAD::wrapGcGetHwVersion(uint32_t *param1)
{
	SYSLOG("rad", "_gc_get_hw_version called!");
	SYSLOG("rad", "_gc_get_hw_version: param1 = %p", param1);
	auto ret = FunctionCast(wrapGcGetHwVersion, callbackRAD->orgGcGetHwVersion)(param1);
	SYSLOG("rad", "_gc_get_hw_version returned 0x%x", ret);
	if ((ret & 0xFF0000) == 0x90000) {
		SYSLOG("rad", "Spoofing GC version 9.x.x to 9.2.1");
		return 0x90201;
	}
	return ret;
}

uint32_t RAD::wrapInternalCosReadFw(uint64_t param1, uint64_t *param2)
{
	SYSLOG("rad", "_internal_cos_read_fw called!");
	SYSLOG("rad", "_internal_cos_read_fw: param1 = 0x%llx param2 = %p", param1, param2);
	auto ret = FunctionCast(wrapInternalCosReadFw, callbackRAD->orgInternalCosReadFw)(param1, param2);
	SYSLOG("rad", "_internal_cos_read_fw returned 0x%x", ret);
	return ret;
}

void RAD::wrapPopulateFirmwareDirectory(void *that)
{
	SYSLOG("rad", "AMDRadeonX5000_AMDRadeonHWLibsX5000::populateFirmwareDirectory called!");
	FunctionCast(wrapPopulateFirmwareDirectory, callbackRAD->orgPopulateFirmwareDirectory)(that);
	SYSLOG("rad", "injecting ativvaxy_rv.dat!");
	auto *fwDesc = getFWDescByName("ativvaxy_rv.dat");
	
	auto *fw = callbackRAD->orgCreateFirmware(fwDesc->getBytesNoCopy(), fwDesc->getLength(), 0x200, "ativvaxy_rv.dat");
	auto *fwDir = *(void **)((uint8_t *)that + 0xB8);
	SYSLOG("rad", "fwDir = %p", fwDir);
	if (!callbackRAD->orgPutFirmware(fwDir, 6, fw)) {
		panic("Failed to inject ativvaxy_rv.dat firmware");
	}
}

bool RAD::wrapIpiSmuIsSwipExcluded()
{
	SYSLOG("rad", "_IpiSmuIsSwipExcluded called!");
	return true;
}

void *RAD::wrapCreateAtomBiosProxy(void *param1)
{
	SYSLOG("rad", "----------------------------------------------------------------------");
	SYSLOG("rad", "createAtomBiosProxy called!");
	SYSLOG("rad", "createAtomBiosProxy: param1 = %p", param1);
	auto ret = FunctionCast(wrapCreateAtomBiosProxy, callbackRAD->orgCreateAtomBiosProxy)(param1);
	SYSLOG("rad", "createAtomBiosProxy returned %p", ret);
	return ret;
}

IOReturn RAD::wrapInitializeResources(void *that)
{
	SYSLOG("rad", "initializeResources called!");
	SYSLOG("rad", "initializeResources: this = %p", that);
	auto ret = FunctionCast(wrapInitializeResources, callbackRAD->orgInitializeResources)(that);
	SYSLOG("rad", "initializeResources returned 0x%x", ret);
	return ret;
}

IOReturn RAD::wrapPopulateDeviceMemory(void *that, uint32_t reg)
{
	DBGLOG("rad", "populateDeviceMemory called!");
	DBGLOG("rad", "populateDeviceMemory: this = %p reg = 0x%x", that, reg);
	auto ret = FunctionCast(wrapPopulateDeviceMemory, callbackRAD->orgPopulateDeviceMemory)(that, reg);
	DBGLOG("rad", "populateDeviceMemory returned 0x%x", ret);
	return kIOReturnSuccess;
}

void *RAD::wrapGetGpuHwConstants(uint8_t *param1)
{
	SYSLOG("rad", "----------------------------------------------------------------------");
	SYSLOG("rad", "_GetGpuHwConstants called!");
	SYSLOG("rad", "_GetGpuHwConstants: param1 = %p", param1);
	auto *asicCaps = *(uint8_t **)(param1 + 0x350);
	SYSLOG("rad", "_GetGpuHwConstants: asicCaps = %p", asicCaps);
	uint16_t deviceId = *(uint16_t *)(asicCaps + 8);
	SYSLOG("rad", "_GetGpuHwConstants: deviceId = 0x%x", deviceId);
	auto *goldenSettings = *(uint8_t **)(asicCaps + 48);
	SYSLOG("rad", "_GetGpuHwConstants: goldenSettings = %p", goldenSettings);
	for (size_t i = 0; i < 24; i++) {
		SYSLOG("rad", "_GetGpuHwConstants: goldenSettings: %d:0x%x", i, goldenSettings[i]);
	}
	auto ret = FunctionCast(wrapGetGpuHwConstants, callbackRAD->orgGetGpuHwConstants)(param1);
	SYSLOG("rad", "_GetGpuHwConstants returned %p", ret);
	SYSLOG("rad", "----------------------------------------------------------------------");
	if (!ret)
	{
		SYSLOG("rad", "_GetGpuHwConstants failed!");
		IOSleep(60000);
		panic("_GetGpuHwConstants returned ZERO value!");
	}
	return ret;
}

bool RAD::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size)
{
	if (kextRadeonFramebuffer.loadIndex == index)
	{
		if (force24BppMode) process24BitOutput(patcher, kextRadeonFramebuffer, address, size);
		
		return true;
	}
	else if (kextRadeonSupport.loadIndex == index)
	{
		processConnectorOverrides(patcher, address, size);
		
		KernelPatcher::RouteRequest requests[] = {
			{"__ZN13ATIController8TestVRAME13PCI_REG_INDEXb", doNotTestVram},
			{"__ZN16AtiDeviceControl16notifyLinkChangeE31kAGDCRegisterLinkControlEvent_tmj", wrapNotifyLinkChange, orgNotifyLinkChange},
			{"__ZN11AtiAsicInfo18initWithControllerEP13ATIController", wrapInitWithController, orgInitWithController},
			{"__ZN23AtiVramInfoInterface_V214createVramInfoEP14AtiVBiosHelperj", createVramInfo},
			{"__ZN13AtomBiosProxy19createAtomBiosProxyER16AtomBiosInitData", wrapCreateAtomBiosProxy, orgCreateAtomBiosProxy},
			{"__ZN13ATIController20populateDeviceMemoryE13PCI_REG_INDEX", wrapPopulateDeviceMemory, orgPopulateDeviceMemory},
		};
		if (!patcher.routeMultiple(index, requests, arrsize(requests), address, size))
			panic("Failed to route AMDSupport symbols");
		
		return true;
	}
	else if (kextRadeonX5000HWLibs.loadIndex == index)
	{
		DBGLOG("rad", "resolving device type table");
		orgDeviceTypeTable = patcher.solveSymbol(index, "__ZL15deviceTypeTable");
		if (!orgDeviceTypeTable)
		{
			panic("RAD: Failed to resolve device type table");
		}
		orgCreateFirmware = reinterpret_cast<t_createFirmware>(patcher.solveSymbol(index, "__ZN11AMDFirmware14createFirmwareEPhjjPKc"));
		if (!this->orgCreateFirmware)
		{
			panic("RAD: Failed to resolve AMDFirmware::createFirmware");
		}
		orgPutFirmware = reinterpret_cast<t_putFirmware>(patcher.solveSymbol(index, "__ZN20AMDFirmwareDirectory11putFirmwareE16_AMD_DEVICE_TYPEP11AMDFirmware"));
		if (!orgPutFirmware)
		{
			panic("RAD: Failed to resolve AMDFirmwareDirectory::putFirmware");
		}
		
		KernelPatcher::RouteRequest requests[] = {
			{"__ZN14AmdTtlServicesC2EP11IOPCIDevice", wrapAmdTtlServicesConstructor, orgAmdTtlServicesConstructor},
			{"_ttlDevSetSmuFwVersion", wrapTtlDevSetSmuFwVersion, orgTtlDevSetSmuFwVersion},
			{"_IpiSetFwEntry", wrapIpiSetFwEntry, orgIpiSetFwEntry},
			{"_ipi_smu_sw_init", wrapIpiSmuSwInit, orgIpiSmuSwInit},
			{"_smu_sw_init", wrapSmuSwInit, orgSmuSwInit},
			{"_smu_internal_sw_init", wrapSmuInternalSwInit, orgSmuInternalSwInit},
			{"_smu_get_hw_version", wrapSmuGetHwVersion, orgSmuGetHwVersion},
			{"_psp_sw_init", wrapPspSwInit, orgPspSwInit},
			{"_gc_get_hw_version", wrapGcGetHwVersion, orgGcGetHwVersion},
			{"_internal_cos_read_fw", wrapInternalCosReadFw, orgInternalCosReadFw},
			{"__ZN35AMDRadeonX5000_AMDRadeonHWLibsX500025populateFirmwareDirectoryEv", wrapPopulateFirmwareDirectory, orgPopulateFirmwareDirectory},
			{"_IpiSmuIsSwipExcluded", wrapIpiSmuIsSwipExcluded},
			{"_GetGpuHwConstants", wrapGetGpuHwConstants, orgGetGpuHwConstants},
		};
		if (!patcher.routeMultiple(index, requests, arrsize(requests), address, size))
			panic("RAD: Failed to route AMDRadeonX5000HWLibs symbols");
		
		return true;
	} else if (kextAMD10000Controller.loadIndex == index) {
		DBGLOG("rad", "Hooking AMD10000Controller");
		
		KernelPatcher::RouteRequest requests[] = {
			{"__ZN18AMD10000Controller23findProjectByPartNumberEP20ControllerProperties", wrapProjectByPartNumber},
			{"__ZN18AMD10000Controller35initializeProjectDependentResourcesEv", wrapInitializeProjectDependentResources, orgInitializeProjectDependentResources},
			{"__ZN18AMD10000Controller21hwInitializeFbMemSizeEv", wrapHwInitializeFbMemSize, orgHwInitializeFbMemSize},
			{"__ZN18AMD10000Controller18hwInitializeFbBaseEv", wrapHwInitializeFbMemSize, orgHwInitializeFbMemSize},
			{"__ZN18AMD10000Controller19initializeResourcesEv", wrapInitializeResources, orgInitializeResources},
		};
		if (!patcher.routeMultiple(index, requests, arrsize(requests), address, size))
			panic("Failed to route AMD10000Controller symbols");
		
		uint8_t find[] = { 0x3d, 0x60, 0x68, 0x00, 0x00, 0x0f, 0x8c, 0x32, 0x00, 0x00, 0x00, 0x0f, 0xb7, 0x45, 0xe6, 0x3d, 0x7f, 0x68, 0x00, 0x00, 0x0f, 0x8f, 0x23, 0x00, 0x00, 0x00, 0xbf, 0x78, 0x00, 0x00, 0x00 };
		uint8_t repl[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x0f, 0xb7, 0x45, 0xe6, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xbf, 0x78, 0x00, 0x00, 0x00 };
		KernelPatcher::LookupPatch patch {&kextAMD10000Controller, find, repl, arrsize(find), 2};
		patcher.applyLookupPatch(&patch);
		patcher.clearError();
		
		return true;
	}
	
	for (size_t i = 0; i < maxHardwareKexts; i++)
	{
		if (kextRadeonHardware[i].loadIndex == index)
		{
			processHardwareKext(patcher, i, address, size);
			return true;
		}
	}
	
	return false;
}

void RAD::initHardwareKextMods()
{
	lilu.onKextLoadForce(kextRadeonHardware, maxHardwareKexts);
}

void RAD::process24BitOutput(KernelPatcher &patcher, KernelPatcher::KextInfo &info, mach_vm_address_t address, size_t size)
{
	auto bitsPerComponent = patcher.solveSymbol<int *>(info.loadIndex, "__ZL18BITS_PER_COMPONENT", address, size);
	if (bitsPerComponent)
	{
		while (bitsPerComponent && *bitsPerComponent)
		{
			if (*bitsPerComponent == 10)
			{
				auto ret = MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock);
				if (ret == KERN_SUCCESS)
				{
					DBGLOG("rad", "fixing BITS_PER_COMPONENT");
					*bitsPerComponent = 8;
					MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
				}
				else
				{
					SYSLOG("rad", "failed to disable write protection for BITS_PER_COMPONENT");
				}
			}
			bitsPerComponent++;
		}
	}
	else
	{
		SYSLOG("rad", "failed to find BITS_PER_COMPONENT");
		patcher.clearError();
	}
	
	DBGLOG("rad", "fixing pixel types");
	
	KernelPatcher::LookupPatch pixelPatch{
		&info,
		reinterpret_cast<const uint8_t *>("--RRRRRRRRRRGGGGGGGGGGBBBBBBBBBB"),
		reinterpret_cast<const uint8_t *>("--------RRRRRRRRGGGGGGGGBBBBBBBB"),
		32, 2};
	
	patcher.applyLookupPatch(&pixelPatch);
	if (patcher.getError() != KernelPatcher::Error::NoError)
	{
		SYSLOG("rad", "failed to patch RGB mask for 24-bit output");
		patcher.clearError();
	}
}

void RAD::processConnectorOverrides(KernelPatcher &patcher, mach_vm_address_t address, size_t size)
{
	KernelPatcher::RouteRequest requests[] = {
		{"__ZN14AtiBiosParser116getConnectorInfoEP13ConnectorInfoRh", wrapGetConnectorsInfoV1, orgGetConnectorsInfoV1},
		{"__ZN14AtiBiosParser216getConnectorInfoEP13ConnectorInfoRh", wrapGetConnectorsInfoV2, orgGetConnectorsInfoV2},
		{
			"__ZN14AtiBiosParser126translateAtomConnectorInfoERN30AtiObjectInfoTableInterface_V117AtomConnectorInfoER13ConnectorInfo",
			wrapTranslateAtomConnectorInfoV1,
			orgTranslateAtomConnectorInfoV1,
		},
		{
			"__ZN14AtiBiosParser226translateAtomConnectorInfoERN30AtiObjectInfoTableInterface_V217AtomConnectorInfoER13ConnectorInfo",
			wrapTranslateAtomConnectorInfoV2,
			orgTranslateAtomConnectorInfoV2,
		},
		{"__ZN13ATIController5startEP9IOService", wrapATIControllerStart, orgATIControllerStart},
		
	};
	patcher.routeMultiple(kextRadeonSupport.loadIndex, requests, address, size);
}

uint64_t RAD::wrapConfigureDevice(void *that, IOPCIDevice *dev)
{
	SYSLOG("rad", "configureDevice called!");
	auto ret = FunctionCast(wrapConfigureDevice, callbackRAD->orgConfigureDevice)(that, dev);
	SYSLOG("rad", "configureDevice returned 0x%x", ret);
	return ret;
}

IOService *RAD::wrapInitLinkToPeer(void *that, const char *matchCategoryName)
{
	SYSLOG("rad", "initLinkToPeer called!");
	auto ret = FunctionCast(wrapInitLinkToPeer, callbackRAD->orgInitLinkToPeer)(that, matchCategoryName);
	SYSLOG("rad", "initLinkToPeer returned 0x%x", ret);
	return ret;
}																		\

WRAP_SIMPLE(uint64_t, CreateHWHandler, "0x%x")
uint64_t RAD::wrapCreateHWInterface(void *that, IOPCIDevice *dev)
{
	SYSLOG("rad", "createHWInterface called!");
	auto ret = FunctionCast(wrapCreateHWInterface, callbackRAD->orgCreateHWInterface)(that, dev);
	SYSLOG("rad", "createHWInterface returned 0x%x", ret);
	return ret;
}
WRAP_SIMPLE(uint64_t, GetHWMemory, "0x%x")
WRAP_SIMPLE(uint64_t, GetATIChipConfigBit, "0x%x")
WRAP_SIMPLE(uint64_t, AllocateAMDHWRegisters, "0x%x")
WRAP_SIMPLE(bool, SetupCAIL, "%d")
WRAP_SIMPLE(uint64_t, InitializeHWWorkarounds, "0x%x")
WRAP_SIMPLE(uint64_t, AllocateAMDHWAlignManager, "0x%x")
WRAP_SIMPLE(bool, MapDoorbellMemory, "%d")
uint64_t RAD::wrapGetState(void* that)
{
	DBGLOG("rad", "getState called!");
	auto ret = FunctionCast(wrapGetState, callbackRAD->orgGetState)(that);
	DBGLOG("rad", "getState returned 0x%llx", ret);
	return ret;
}

uint32_t RAD::wrapInitializeTtl(void *that, void *param1)
{
	SYSLOG("rad", "initializeTtl called!");
	auto ret = FunctionCast(wrapInitializeTtl, callbackRAD->orgInitializeTtl)(that, param1);
	SYSLOG("rad", "initializeTtl returned 0x%x", ret);
	return ret;
}

WRAP_SIMPLE(uint64_t, ConfRegBase, "0x%llx")
WRAP_SIMPLE(uint8_t, ReadChipRev, "%d")

void RAD::processHardwareKext(KernelPatcher &patcher, size_t hwIndex, mach_vm_address_t address, size_t size)
{
	auto &hardware = kextRadeonHardware[hwIndex];
	
	KernelPatcher::RouteRequest requests[] = {
		{"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator15configureDeviceEP11IOPCIDevice", wrapConfigureDevice, orgConfigureDevice},
		{"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator14initLinkToPeerEPKc", wrapInitLinkToPeer, orgInitLinkToPeer},
		{"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator15createHWHandlerEv", wrapCreateHWHandler, orgCreateHWHandler},
		{"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator17createHWInterfaceEP11IOPCIDevice", wrapCreateHWInterface, orgCreateHWInterface},
		{"__ZN26AMDRadeonX5000_AMDHardware11getHWMemoryEv", wrapGetHWMemory, orgGetHWMemory},
		{"__ZN32AMDRadeonX5000_AMDVega10Hardware19getATIChipConfigBitEv", wrapGetATIChipConfigBit, orgGetATIChipConfigBit},
		{"__ZN26AMDRadeonX5000_AMDHardware22allocateAMDHWRegistersEv", wrapAllocateAMDHWRegisters, orgAllocateAMDHWRegisters},
		{"__ZN30AMDRadeonX5000_AMDGFX9Hardware23initializeHWWorkaroundsEv", wrapInitializeHWWorkarounds, orgInitializeHWWorkarounds},
		{"__ZN30AMDRadeonX5000_AMDGFX9Hardware25allocateAMDHWAlignManagerEv", wrapAllocateAMDHWAlignManager, orgAllocateAMDHWAlignManager},
		{"__ZN26AMDRadeonX5000_AMDHardware17mapDoorbellMemoryEv", wrapMapDoorbellMemory, orgMapDoorbellMemory},
		{"__ZN27AMDRadeonX5000_AMDHWHandler8getStateEv", wrapGetState, orgGetState},
		{"__ZN28AMDRadeonX5000_AMDRTHardware13initializeTtlEP16_GART_PARAMETERS", wrapInitializeTtl, orgInitializeTtl},
		{"__ZN28AMDRadeonX5000_AMDRTHardware22configureRegisterBasesEv", wrapConfRegBase, orgConfRegBase},
		{"__ZN32AMDRadeonX5000_AMDVega10Hardware23readChipRevFromRegisterEv", wrapReadChipRev, orgReadChipRev},
	};
	patcher.routeMultiple(hardware.loadIndex, requests, arrsize(requests), address, size);
	
		// Patch AppleGVA support for non-supported models
	if (forceCodecInfo && getHWInfoProcNames[hwIndex] != nullptr)
	{
		KernelPatcher::RouteRequest request(getHWInfoProcNames[hwIndex], wrapGetHWInfo[hwIndex], orgGetHWInfo[hwIndex]);
		patcher.routeMultiple(hardware.loadIndex, &request, 1, address, size);
	}
}

void RAD::mergeProperty(OSDictionary *props, const char *name, OSObject *value)
{
		// The only type we could make from device properties is data.
		// To be able to override other types we do a conversion here.
	auto data = OSDynamicCast(OSData, value);
	if (data)
	{
			// It is hard to make a boolean even from ACPI, so we make a hack here:
			// 1-byte OSData with 0x01 / 0x00 values becomes boolean.
		auto val = static_cast<const uint8_t *>(data->getBytesNoCopy());
		auto len = data->getLength();
		if (val && len == sizeof(uint8_t))
		{
			if (val[0] == 1)
			{
				props->setObject(name, kOSBooleanTrue);
				DBGLOG("rad", "prop %s was merged as kOSBooleanTrue", name);
				return;
			}
			else if (val[0] == 0)
			{
				props->setObject(name, kOSBooleanFalse);
				DBGLOG("rad", "prop %s was merged as kOSBooleanFalse", name);
				return;
			}
		}
		
			// Consult the original value to make a decision
		auto orgValue = props->getObject(name);
		if (val && orgValue)
		{
			DBGLOG("rad", "prop %s has original value", name);
			if (len == sizeof(uint32_t) && OSDynamicCast(OSNumber, orgValue))
			{
				auto num = *reinterpret_cast<const uint32_t *>(val);
				auto osnum = OSNumber::withNumber(num, 32);
				if (osnum)
				{
					DBGLOG("rad", "prop %s was merged as number %u", name, num);
					props->setObject(name, osnum);
					osnum->release();
				}
				return;
			}
			else if (len > 0 && val[len - 1] == '\0' && OSDynamicCast(OSString, orgValue))
			{
				auto str = reinterpret_cast<const char *>(val);
				auto osstr = OSString::withCString(str);
				if (osstr)
				{
					DBGLOG("rad", "prop %s was merged as string %s", name, str);
					props->setObject(name, osstr);
					osstr->release();
				}
				return;
			}
		}
		else
		{
			DBGLOG("rad", "prop %s has no original value", name);
		}
	}
	
		// Default merge as is
	props->setObject(name, value);
	DBGLOG("rad", "prop %s was merged", name);
}

void RAD::mergeProperties(OSDictionary *props, const char *prefix, IOService *provider)
{
		// Should be ok, but in case there are issues switch to dictionaryWithProperties();
	auto dict = provider->getPropertyTable();
	if (dict)
	{
		auto iterator = OSCollectionIterator::withCollection(dict);
		if (iterator)
		{
			OSSymbol *propname;
			size_t prefixlen = strlen(prefix);
			while ((propname = OSDynamicCast(OSSymbol, iterator->getNextObject())) != nullptr)
			{
				auto name = propname->getCStringNoCopy();
				if (name && propname->getLength() > prefixlen && !strncmp(name, prefix, prefixlen))
				{
					auto prop = dict->getObject(propname);
					if (prop)
						mergeProperty(props, name + prefixlen, prop);
					else {
						DBGLOG("rad", "prop %s was not merged due to no value", name);
					}
				}
				else
				{
					DBGLOG("rad", "prop %s does not match %s prefix", safeString(name), prefix);
				}
			}
			
			iterator->release();
		}
		else
		{
			SYSLOG("rad", "prop merge failed to iterate over properties");
		}
	}
	else
	{
		SYSLOG("rad", "prop merge failed to get properties");
	}
	
	if (!strcmp(prefix, "CAIL,"))
	{
		for (size_t i = 0; i < arrsize(powerGatingFlags); i++)
		{
			if (powerGatingFlags[i] && props->getObject(powerGatingFlags[i]))
			{
				DBGLOG("rad", "cail prop merge found %s, replacing", powerGatingFlags[i]);
				auto num = OSNumber::withNumber(1, 32);
				if (num)
				{
					props->setObject(powerGatingFlags[i], num);
					num->release();
				}
			}
		}
	}
}

void RAD::applyPropertyFixes(IOService *service, uint32_t connectorNum)
{
	if (!service->getProperty("CFG,CFG_FB_LIMIT"))
	{
		DBGLOG("rad", "setting fb limit to %u", connectorNum);
		service->setProperty("CFG_FB_LIMIT", connectorNum, 32);
	}
}

uint32_t RAD::wrapGetConnectorsInfoV1(void *that, RADConnectors::Connector *connectors, uint8_t *sz)
{
	uint32_t code = FunctionCast(wrapGetConnectorsInfoV1, callbackRAD->orgGetConnectorsInfoV1)(that, connectors, sz);
	auto props = callbackRAD->currentPropProvider.get();
	
	if (code == 0 && sz && props && *props)
	{
		callbackRAD->updateConnectorsInfo(nullptr, nullptr, *props, connectors, sz);
	}
	else
		SYSLOG("rad", "getConnectorsInfoV1 failed %X or undefined %d", code, props == nullptr);
	
	return code;
}

void RAD::updateConnectorsInfo(void *atomutils, t_getAtomObjectTableForType gettable, IOService *ctrl, RADConnectors::Connector *connectors, uint8_t *sz)
{
	if (atomutils)
	{
		SYSLOG("rad", "getConnectorsInfo found %u connectors", *sz);
		RADConnectors::print(connectors, *sz);
	}
	
	auto cons = ctrl->getProperty("connectors");
	if (cons)
	{
		auto consData = OSDynamicCast(OSData, cons);
		if (consData)
		{
			auto consPtr = consData->getBytesNoCopy();
			auto consSize = consData->getLength();
			
			uint32_t consCount;
			if (WIOKit::getOSDataValue(ctrl, "connector-count", consCount))
			{
				*sz = consCount;
				SYSLOG("rad", "getConnectorsInfo got size override to %u", *sz);
			}
			
			if (consPtr && consSize > 0 && *sz > 0 && RADConnectors::valid(consSize, *sz))
			{
				RADConnectors::copy(connectors, *sz, static_cast<const RADConnectors::Connector *>(consPtr), consSize);
				SYSLOG("rad", "getConnectorsInfo installed %u connectors", *sz);
				applyPropertyFixes(ctrl, *sz);
			}
			else
			{
				SYSLOG("rad", "getConnectorsInfo conoverrides have invalid size %u for %u num", consSize, *sz);
			}
		}
		else
		{
			SYSLOG("rad", "getConnectorsInfo conoverrides have invalid type");
		}
	}
	else
	{
		if (atomutils)
		{
			SYSLOG("rad", "getConnectorsInfo attempting to autofix connectors");
			uint8_t sHeader = 0, displayPathNum = 0, connectorObjectNum = 0;
			auto baseAddr = static_cast<uint8_t *>(gettable(atomutils, AtomObjectTableType::Common, &sHeader)) - sizeof(uint32_t);
			auto displayPaths = static_cast<AtomDisplayObjectPath *>(gettable(atomutils, AtomObjectTableType::DisplayPath, &displayPathNum));
			auto connectorObjects = static_cast<AtomConnectorObject *>(gettable(atomutils, AtomObjectTableType::ConnectorObject, &connectorObjectNum));
			if (displayPathNum == connectorObjectNum)
			{
				autocorrectConnectors(baseAddr, displayPaths, displayPathNum, connectorObjects, connectorObjectNum, connectors, *sz);
			}
			else
			{
				SYSLOG("rad", "getConnectorsInfo found different displaypaths %u and connectors %u", displayPathNum, connectorObjectNum);
			}
		}
		
		applyPropertyFixes(ctrl, *sz);
		
		const uint8_t *senseList = nullptr;
		uint8_t senseNum = 0;
		auto priData = OSDynamicCast(OSData, ctrl->getProperty("connector-priority"));
		if (priData)
		{
			senseList = static_cast<const uint8_t *>(priData->getBytesNoCopy());
			senseNum = static_cast<uint8_t>(priData->getLength());
			SYSLOG("rad", "getConnectorInfo found %u senses in connector-priority", senseNum);
			reprioritiseConnectors(senseList, senseNum, connectors, *sz);
		}
		else
		{
			SYSLOG("rad", "getConnectorInfo leaving unchaged priority");
		}
	}
	
	SYSLOG("rad", "getConnectorsInfo resulting %u connectors follow", *sz);
	RADConnectors::print(connectors, *sz);
}

uint32_t RAD::wrapTranslateAtomConnectorInfoV1(void *that, RADConnectors::AtomConnectorInfo *info, RADConnectors::Connector *connector)
{
	uint32_t code = FunctionCast(wrapTranslateAtomConnectorInfoV1, callbackRAD->orgTranslateAtomConnectorInfoV1)(that, info, connector);
	
	if (code == 0 && info && connector)
	{
		RADConnectors::print(connector, 1);
		
		uint8_t sense = getSenseID(info->i2cRecord);
		if (sense)
		{
			SYSLOG("rad", "translateAtomConnectorInfoV1 got sense id %02X", sense);
			
				// We need to extract usGraphicObjIds from info->hpdRecord, which is of type ATOM_SRC_DST_TABLE_FOR_ONE_OBJECT:
				// struct ATOM_SRC_DST_TABLE_FOR_ONE_OBJECT {
				//   uint8_t ucNumberOfSrc;
				//   uint16_t usSrcObjectID[ucNumberOfSrc];
				//   uint8_t ucNumberOfDst;
				//   uint16_t usDstObjectID[ucNumberOfDst];
				// };
				// The value we need is in usSrcObjectID. The structure is byte-packed.
			
			uint8_t ucNumberOfSrc = info->hpdRecord[0];
			for (uint8_t i = 0; i < ucNumberOfSrc; i++)
			{
				auto usSrcObjectID = *reinterpret_cast<uint16_t *>(info->hpdRecord + sizeof(uint8_t) + i * sizeof(uint16_t));
				SYSLOG("rad", "translateAtomConnectorInfoV1 checking %04X object id", usSrcObjectID);
				if (((usSrcObjectID & OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT) == GRAPH_OBJECT_TYPE_ENCODER)
				{
					uint8_t txmit = 0, enc = 0;
					if (getTxEnc(usSrcObjectID, txmit, enc))
						callbackRAD->autocorrectConnector(getConnectorID(info->usConnObjectId), getSenseID(info->i2cRecord), txmit, enc, connector, 1);
					break;
				}
			}
		}
		else
		{
			SYSLOG("rad", "translateAtomConnectorInfoV1 failed to detect sense for translated connector");
		}
	}
	
	return code;
}


void RAD::autocorrectConnectors(uint8_t *baseAddr, AtomDisplayObjectPath *displayPaths, uint8_t displayPathNum, AtomConnectorObject *connectorObjects,
								[[maybe_unused]] uint8_t connectorObjectNum, RADConnectors::Connector *connectors, uint8_t sz)
{
	for (uint8_t i = 0; i < displayPathNum; i++)
	{
		if (!isEncoder(displayPaths[i].usGraphicObjIds))
		{
			SYSLOG("rad", "autocorrectConnectors not encoder %X at %u", displayPaths[i].usGraphicObjIds, i);
			continue;
		}
		
		uint8_t txmit = 0, enc = 0;
		if (!getTxEnc(displayPaths[i].usGraphicObjIds, txmit, enc))
		{
			continue;
		}
		
		uint8_t sense = getSenseID(baseAddr + connectorObjects[i].usRecordOffset);
		if (!sense)
		{
			SYSLOG("rad", "autocorrectConnectors failed to detect sense for %u connector", i);
			continue;
		}
		
		SYSLOG("rad", "autocorrectConnectors found txmit %02X enc %02X sense %02X for %u connector", txmit, enc, sense, i);
		
		autocorrectConnector(getConnectorID(displayPaths[i].usConnObjectId), sense, txmit, enc, connectors, sz);
	}
}

void RAD::autocorrectConnector(uint8_t connector, uint8_t sense, uint8_t txmit, [[maybe_unused]] uint8_t enc, RADConnectors::Connector *connectors, uint8_t sz)
{
	if (callbackRAD->dviSingleLink)
	{
		if (connector != CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_I &&
			connector != CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_D &&
			connector != CONNECTOR_OBJECT_ID_LVDS)
		{
			SYSLOG("rad", "autocorrectConnector found unsupported connector type %02X", connector);
			return;
		}
		
		auto fixTransmit = [](auto &con, uint8_t idx, uint8_t sense, uint8_t txmit)
		{
			if (con.sense == sense)
			{
				if (con.transmitter != txmit && (con.transmitter & 0xCF) == con.transmitter)
				{
					SYSLOG("rad", "autocorrectConnector replacing txmit %02X with %02X for %u connector sense %02X",
						   con.transmitter, txmit, idx, sense);
					con.transmitter = txmit;
				}
				return true;
			}
			return false;
		};
		
		bool isModern = RADConnectors::modern();
		for (uint8_t j = 0; j < sz; j++)
		{
			if (isModern)
			{
				auto &con = (&connectors->modern)[j];
				if (fixTransmit(con, j, sense, txmit))
					break;
			}
			else
			{
				auto &con = (&connectors->legacy)[j];
				if (fixTransmit(con, j, sense, txmit))
					break;
			}
		}
	}
	else
		SYSLOG("rad", "autocorrectConnector use -raddvi to enable dvi autocorrection");
}

void RAD::reprioritiseConnectors(const uint8_t *senseList, uint8_t senseNum, RADConnectors::Connector *connectors, uint8_t sz)
{
	static constexpr uint32_t typeList[] = {
		RADConnectors::ConnectorLVDS,
		RADConnectors::ConnectorDigitalDVI,
		RADConnectors::ConnectorHDMI,
		RADConnectors::ConnectorDP,
		RADConnectors::ConnectorVGA,
	};
	static constexpr uint8_t typeNum{static_cast<uint8_t>(arrsize(typeList))};
	
	bool isModern = RADConnectors::modern();
	uint16_t priCount = 1;
	for (uint8_t i = 0; i < senseNum + typeNum + 1; i++)
	{
		for (uint8_t j = 0; j < sz; j++)
		{
			auto reorder = [&](auto &con)
			{
				if (i == senseNum + typeNum)
				{
					if (con.priority == 0)
						con.priority = priCount++;
				}
				else if (i < senseNum)
				{
					if (con.sense == senseList[i])
					{
						SYSLOG("rad", "reprioritiseConnectors setting priority of sense %02X to %u by sense", con.sense, priCount);
						con.priority = priCount++;
						return true;
					}
				}
				else
				{
					if (con.priority == 0 && con.type == typeList[i - senseNum])
					{
						SYSLOG("rad", "reprioritiseConnectors setting priority of sense %02X to %u by type", con.sense, priCount);
						con.priority = priCount++;
					}
				}
				return false;
			};
			
			if ((isModern && reorder((&connectors->modern)[j])) ||
				(!isModern && reorder((&connectors->legacy)[j])))
			{
				break;
			}
		}
	}
}

void RAD::updateAccelConfig([[maybe_unused]] size_t hwIndex, IOService *accelService, const char **accelConfig)
{
	if (accelService && accelConfig)
	{
		if (fixConfigName)
		{
			auto gpuService = accelService->getParentEntry(gIOServicePlane);
			
			if (gpuService)
			{
				auto model = OSDynamicCast(OSData, gpuService->getProperty("model"));
				if (model)
				{
					auto modelStr = static_cast<const char *>(model->getBytesNoCopy());
					if (modelStr)
					{
						if (modelStr[0] == 'A' && ((modelStr[1] == 'M' && modelStr[2] == 'D') || (modelStr[1] == 'T' && modelStr[2] == 'I')) && modelStr[3] == ' ')
							modelStr += 4;
						
						DBGLOG("rad", "updateAccelConfig found gpu model %s", modelStr);
						*accelConfig = modelStr;
					}
					else
						DBGLOG("rad", "updateAccelConfig found null gpu model");
				}
				else
					DBGLOG("rad", "updateAccelConfig failed to find gpu model");
			}
			else
				DBGLOG("rad", "updateAccelConfig failed to find accelerator parent");
		}
	}
}

bool RAD::wrapSetProperty(IORegistryEntry *that, const char *aKey, void *bytes, unsigned length)
{
	if (length > 10 && aKey && reinterpret_cast<const uint32_t *>(aKey)[0] == 'edom' && reinterpret_cast<const uint16_t *>(aKey)[2] == 'l')
	{
		DBGLOG("rad", "SetProperty caught model %u (%.*s)", length, length, static_cast<char *>(bytes));
		if (*static_cast<uint32_t *>(bytes) == ' DMA' || *static_cast<uint32_t *>(bytes) == ' ITA' || *static_cast<uint32_t *>(bytes) == 'edaR')
		{
			if (FunctionCast(wrapGetProperty, callbackRAD->orgGetProperty)(that, aKey))
			{
				DBGLOG("rad", "SetProperty ignored setting %s to %s", aKey, static_cast<char *>(bytes));
				return true;
			}
			DBGLOG("rad", "SetProperty missing %s, fallback to %s", aKey, static_cast<char *>(bytes));
		}
	}
	
	return FunctionCast(wrapSetProperty, callbackRAD->orgSetProperty)(that, aKey, bytes, length);
}

OSObject *RAD::wrapGetProperty(IORegistryEntry *that, const char *aKey)
{
	auto obj = FunctionCast(wrapGetProperty, callbackRAD->orgGetProperty)(that, aKey);
	auto props = OSDynamicCast(OSDictionary, obj);
	
	if (props && aKey)
	{
		const char *prefix{nullptr};
		auto provider = OSDynamicCast(IOService, that->getParentEntry(gIOServicePlane));
		if (provider)
		{
			if (aKey[0] == 'a')
			{
				if (!strcmp(aKey, "aty_config"))
					prefix = "CFG,";
				else if (!strcmp(aKey, "aty_properties"))
					prefix = "PP,";
			}
			else if (aKey[0] == 'c' && !strcmp(aKey, "cail_properties"))
			{
				prefix = "CAIL,";
			}
			
			if (prefix)
			{
				DBGLOG("rad", "GetProperty discovered property merge request for %s", aKey);
				auto rawProps = props->copyCollection();
				if (rawProps)
				{
					auto newProps = OSDynamicCast(OSDictionary, rawProps);
					if (newProps)
					{
						callbackRAD->mergeProperties(newProps, prefix, provider);
						that->setProperty(aKey, newProps);
						obj = newProps;
					}
					rawProps->release();
				}
			}
		}
	}
	
	return obj;
}

uint32_t RAD::wrapGetConnectorsInfoV2(void *that, RADConnectors::Connector *connectors, uint8_t *sz)
{
	uint32_t code = FunctionCast(wrapGetConnectorsInfoV2, callbackRAD->orgGetConnectorsInfoV2)(that, connectors, sz);
	auto props = callbackRAD->currentPropProvider.get();
	
	if (code == 0 && sz && props && *props)
		callbackRAD->updateConnectorsInfo(nullptr, nullptr, *props, connectors, sz);
	else
		SYSLOG("rad", "getConnectorsInfoV2 failed %X or undefined %d", code, props == nullptr);
	
	return code;
}

uint32_t RAD::wrapTranslateAtomConnectorInfoV2(void *that, RADConnectors::AtomConnectorInfo *info, RADConnectors::Connector *connector)
{
	uint32_t code = FunctionCast(wrapTranslateAtomConnectorInfoV2, callbackRAD->orgTranslateAtomConnectorInfoV2)(that, info, connector);
	
	if (code == 0 && info && connector)
	{
		RADConnectors::print(connector, 1);
		
		uint8_t sense = getSenseID(info->i2cRecord);
		if (sense)
		{
			SYSLOG("rad", "translateAtomConnectorInfoV2 got sense id %02X", sense);
			uint8_t txmit = 0, enc = 0;
			if (getTxEnc(info->usGraphicObjIds, txmit, enc))
				callbackRAD->autocorrectConnector(getConnectorID(info->usConnObjectId), getSenseID(info->i2cRecord), txmit, enc, connector, 1);
		}
		else
		{
			SYSLOG("rad", "translateAtomConnectorInfoV2 failed to detect sense for translated connector");
		}
	}
	
	return code;
}

bool RAD::wrapATIControllerStart(IOService *ctrl, IOService *provider)
{
	SYSLOG("rad", "starting controller " PRIKADDR, CASTKADDR(current_thread()));
	if (callbackRAD->forceVesaMode)
	{
		SYSLOG("rad", "disabling video acceleration on request");
		return false;
	}
	
	callbackRAD->currentPropProvider.set(provider);
	bool r = FunctionCast(wrapATIControllerStart, callbackRAD->orgATIControllerStart)(ctrl, provider);
	SYSLOG("rad", "starting controller done %d " PRIKADDR, r, CASTKADDR(current_thread()));
	callbackRAD->currentPropProvider.erase();
	
	return r;
}

bool RAD::doNotTestVram([[maybe_unused]] IOService * ctrl, [[maybe_unused]] uint32_t reg, [[maybe_unused]] bool retryOnFail)
{
	SYSLOG("rad", "TestVRAM called! Returning true");
	return true;
}

bool RAD::wrapNotifyLinkChange(void *atiDeviceControl, kAGDCRegisterLinkControlEvent_t event, void *eventData, uint32_t eventFlags)
{
	auto ret = FunctionCast(wrapNotifyLinkChange, callbackRAD->orgNotifyLinkChange)(atiDeviceControl, event, eventData, eventFlags);
	
	if (event == kAGDCValidateDetailedTiming)
	{
		auto cmd = static_cast<AGDCValidateDetailedTiming_t *>(eventData);
		SYSLOG("rad", "AGDCValidateDetailedTiming %u -> %d (%u)", cmd->framebufferIndex, ret, cmd->modeStatus);
		if (ret == false || cmd->modeStatus < 1 || cmd->modeStatus > 3)
		{
			cmd->modeStatus = 2;
			ret = true;
		}
	}
	
	return ret;
}

void RAD::updateGetHWInfo(IOService *accelVideoCtx, void *hwInfo)
{
	IOService *accel, *pciDev;
	accel = OSDynamicCast(IOService, accelVideoCtx->getParentEntry(gIOServicePlane));
	if (accel == NULL)
	{
		SYSLOG("rad", "getHWInfo: no parent found for accelVideoCtx!");
		return;
	}
	pciDev = OSDynamicCast(IOService, accel->getParentEntry(gIOServicePlane));
	if (pciDev == NULL)
	{
		SYSLOG("rad", "getHWInfo: no parent found for accel!");
		return;
	}
	uint16_t &org = getMember<uint16_t>(hwInfo, 0x4);
	uint32_t dev = org;
	if (!WIOKit::getOSDataValue(pciDev, "codec-device-id", dev))
	{
		WIOKit::getOSDataValue(pciDev, "device-id", dev);
	}
	SYSLOG("rad", "getHWInfo: original PID: 0x%04X, replaced PID: 0x%04X", org, dev);
	org = static_cast<uint16_t>(dev);
}
