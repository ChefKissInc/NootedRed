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
#include <Headers/kern_file.hpp>
#include <IOKit/IOService.h>

#include <Availability.h>
#include <IOKit/IOPlatformExpert.h>

#include "kern_rad.hpp"
#include "kern_fw.hpp"
#include "kern_netdbg.hpp"

#define WRAP_SIMPLE(ty, func, fmt)											\
	ty RAD::wrap##func(void* that)											\
	{																		\
		NETDBG::printf("rad: " #func " called!");							\
		auto ret = FunctionCast(wrap##func, callbackRAD->org##func)(that);	\
		NETDBG::printf("rad: " #func " returned " fmt, ret);				\
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

[[noreturn]] [[gnu::cold]] void RAD::wrapPanic(const char *fmt, ...)
{
	va_list args, netdbg_args;
	va_start(args, fmt);
	va_copy(netdbg_args, args);
	NETDBG::printf("panic: ");
	NETDBG::vprintf(fmt, netdbg_args);
	va_end(netdbg_args);
	IOSleep(1000);
	FunctionCast(wrapPanic, callbackRAD->orgPanic)(fmt, args);
	while (true) {}
}

[[noreturn]] [[gnu::cold]] void RAD::wrapEnterDebugger(const char *cause)
{
	NETDBG::printf("rad: Debugger requested: %s", cause);
	panic("Debugger requested");
}

void RAD::wrapIOLog(const char *fmt, ...)
{
	va_list args, netdbg_args;
	va_start(args, fmt);
	va_copy(netdbg_args, args);
	NETDBG::vprintf(fmt, netdbg_args);
	va_end(netdbg_args);
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
		{"__ZN15IORegistryEntry11setPropertyEPKcPvj", wrapSetProperty, orgSetProperty},
		{"__ZNK15IORegistryEntry11getPropertyEPKc", wrapGetProperty, orgGetProperty},
		{"_panic", wrapPanic, orgPanic},
		{"_PE_enter_debugger", wrapEnterDebugger},
		{"_IOLog", wrapIOLog},
	};
	if (!patcher.routeMultipleLong(KernelPatcher::KernelID, requests)) {
		panic("Failed to route kernel symbols");
	}
}

IOReturn RAD::wrapProjectByPartNumber([[maybe_unused]] IOService* that, [[maybe_unused]] uint64_t partNumber) {
	return kIOReturnNotFound;
}

WRAP_SIMPLE(IOReturn, InitializeProjectDependentResources, "0x%X")
WRAP_SIMPLE(IOReturn, HwInitializeFbMemSize, "0x%X")
WRAP_SIMPLE(IOReturn, HwInitializeFbBase, "0x%X")

uint64_t RAD::wrapInitWithController(void *that, void *controller)
{
	NETDBG::printf("initWithController called!");
	NETDBG::printf("rad: initWithController called!");
	auto ret = FunctionCast(wrapInitWithController, callbackRAD->orgInitWithController)(that, controller);
	NETDBG::printf("rad: initWithController returned %llX", ret);
	return ret;
}

IntegratedVRAMInfoInterface *RAD::createVramInfo([[maybe_unused]] void *helper, [[maybe_unused]] uint32_t offset)
{
	NETDBG::printf("rad: ----------------------------------------------------------------------");
	NETDBG::printf("rad: creating fake VRAM info, get rekt ayymd");
	NETDBG::printf("rad: createVramInfo offset = 0x%X", offset);
	DataTableInitInfo initInfo {
		.helper = helper,
		.tableOffset = offset,
		.revision = AtiAtomDataRevision {
			.formatRevision = 2,
			.contentRevision = 3,
		},
	};
	auto *ret = new IntegratedVRAMInfoInterface;
	ret->init(&initInfo);
	NETDBG::printf("rad: ----------------------------------------------------------------------");
	return ret;
}

void RAD::wrapAmdTtlServicesConstructor(IOService *that, IOPCIDevice *provider)
{
	NETDBG::enabled = true;
	NETDBG::printf("rad: patching device type table");
	MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock);
	*(uint32_t *)callbackRAD->orgDeviceTypeTable = provider->extendedConfigRead16(kIOPCIConfigDeviceID);
	*((uint32_t *)callbackRAD->orgDeviceTypeTable + 1) = 6;
	MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
	
	NETDBG::printf("rad: calling original AmdTtlServices constructor");
	FunctionCast(wrapAmdTtlServicesConstructor, callbackRAD->orgAmdTtlServicesConstructor)(that, provider);
}

uint64_t RAD::wrapIpiSetFwEntry(void *tlsInstance, void *b)
{
	DBGLOG("rad", "_IpiSetFwEntry called!");
	DBGLOG("rad", "_IpiSetFwEntry: tlsInstance = %p param2 = %p", tlsInstance, b);
	auto ret = FunctionCast(wrapIpiSetFwEntry, callbackRAD->orgIpiSetFwEntry)(tlsInstance, b);
	DBGLOG("rad", "_IpiSetFwEntry returned 0x%llX", ret);
	return ret;
}

uint64_t RAD::wrapIpiSmuSwInit(void *tlsInstance)
{
	NETDBG::printf("rad: _ipi_smu_sw_init called!");
	NETDBG::printf("rad: _ipi_smu_sw_init: tlsInstance = %p", tlsInstance);
	auto ret = FunctionCast(wrapIpiSmuSwInit, callbackRAD->orgIpiSmuSwInit)(tlsInstance);
	NETDBG::printf("rad: _ipi_smu_sw_init returned 0x%llX", ret);
	return ret;
}

uint64_t RAD::wrapSmuSwInit(void *input, uint64_t *output)
{
	NETDBG::printf("rad: _smu_sw_init called!");
	NETDBG::printf("rad: _smu_sw_init: input = %p output = %p", input, output);
	auto ret = FunctionCast(wrapSmuSwInit, callbackRAD->orgSmuSwInit)(input, output);
	NETDBG::printf("rad: _smu_sw_init: output 0:0x%llX 1:0x%llX", output[0], output[1]);
	NETDBG::printf("rad: _smu_sw_init returned 0x%llX", ret);
	return ret;
}

uint32_t RAD::wrapSmuInternalSwInit(uint64_t param1, uint64_t param2, void *param3)
{
	NETDBG::printf("rad: _smu_internal_sw_init called!");
	NETDBG::printf("rad: _smu_internal_sw_init: param1 = 0x%llX param2 = 0x%llX param3 = %p", param1, param2, param3);
	auto ret = FunctionCast(wrapSmuInternalSwInit, callbackRAD->orgSmuInternalSwInit)(param1, param2, param3);
	NETDBG::printf("rad: _smu_internal_sw_init returned 0x%X", ret);
	return ret;
}

uint64_t RAD::wrapSmuGetHwVersion(uint64_t param1, uint32_t param2)
{
	NETDBG::printf("rad: _smu_get_hw_version called!");
	NETDBG::printf("rad: _smu_get_hw_version: param1 = 0x%llX param2 = 0x%X", param1, param2);
	auto ret = FunctionCast(wrapSmuGetHwVersion, callbackRAD->orgSmuGetHwVersion)(param1, param2);
	NETDBG::printf("rad: _smu_get_hw_version returned 0x%llX", ret);
	switch (ret)
	{
		case 0x2:
			NETDBG::printf("rad: Spoofing SMU v10 to v9");
			return 0x1;
		case 0xB:
			[[fallthrough]];
		case 0xC:
			NETDBG::printf("rad: Spoofing SMU v11/v12 to v11");
			return 0x3;
		default:
			return ret;
	}
}

uint64_t RAD::wrapPspSwInit(uint32_t *param1, uint32_t *param2)
{
	NETDBG::printf("rad: _psp_sw_init called!");
	NETDBG::printf("rad: _psp_sw_init: param1 = %p param2 = %p", param1, param2);
	NETDBG::printf("rad: _psp_sw_init: param1: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X", param1[0], param1[1], param1[2], param1[3], param1[4], param1[5]);
	switch (param1[3]) {
		case 0xA:
			NETDBG::printf("rad: Spoofing PSP v10 to v9");
			param1[3] = 0x9;
			param1[4] = 0x0;
			param1[5] = 0x0;
			break;
		case 0xB:
			[[fallthrough]];
		case 0xC:
			NETDBG::printf("rad: Spoofing PSP version 11/12 to 11");
			param1[3] = 0xB;
			param1[4] = 0x0;
			param1[5] = 0x0;
			break;
		default:
			break;
	}
	auto ret = FunctionCast(wrapPspSwInit, callbackRAD->orgPspSwInit)(param1, param2);
	NETDBG::printf("rad: _psp_sw_init returned 0x%llX", ret);
	return ret;
}

uint32_t RAD::wrapGcGetHwVersion(uint32_t *param1)
{
	NETDBG::printf("rad: _gc_get_hw_version called!");
	NETDBG::printf("rad: _gc_get_hw_version: param1 = %p", param1);
	auto ret = FunctionCast(wrapGcGetHwVersion, callbackRAD->orgGcGetHwVersion)(param1);
	NETDBG::printf("rad: _gc_get_hw_version returned 0x%X", ret);
	if ((ret & 0xFF0000) == 0x90000) {
		NETDBG::printf("rad: Spoofing GC version 9.x.x to 9.2.1");
		return 0x90201;
	}
	return ret;
}

uint32_t RAD::wrapInternalCosReadFw(uint64_t param1, uint64_t *param2)
{
	NETDBG::printf("rad: _internal_cos_read_fw called!");
	NETDBG::printf("rad: _internal_cos_read_fw: param1 = 0x%llX param2 = %p", param1, param2);
	auto ret = FunctionCast(wrapInternalCosReadFw, callbackRAD->orgInternalCosReadFw)(param1, param2);
	NETDBG::printf("rad: _internal_cos_read_fw returned 0x%X", ret);
	return ret;
}

void RAD::wrapPopulateFirmwareDirectory(void *that)
{
	NETDBG::printf("rad: AMDRadeonX5000_AMDRadeonHWLibsX5000::populateFirmwareDirectory called!");
	FunctionCast(wrapPopulateFirmwareDirectory, callbackRAD->orgPopulateFirmwareDirectory)(that);
	NETDBG::printf("rad: injecting ativvaxy_rv.dat!");
	auto *fwDesc = getFWDescByName("ativvaxy_rv.dat");
	
	auto *fw = callbackRAD->orgCreateFirmware(fwDesc->getBytesNoCopy(), fwDesc->getLength(), 0x200, "ativvaxy_rv.dat");
	auto *fwDir = *(void **)((uint8_t *)that + 0xB8);
	NETDBG::printf("rad: fwDir = %p", fwDir);
	if (!callbackRAD->orgPutFirmware(fwDir, 6, fw)) {
		panic("Failed to inject ativvaxy_rv.dat firmware");
	}
}

void *RAD::wrapCreateAtomBiosProxy(void *param1)
{
	NETDBG::printf("rad: ----------------------------------------------------------------------");
	NETDBG::printf("rad: createAtomBiosProxy called!");
	NETDBG::printf("rad: createAtomBiosProxy: param1 = %p", param1);
	auto ret = FunctionCast(wrapCreateAtomBiosProxy, callbackRAD->orgCreateAtomBiosProxy)(param1);
	NETDBG::printf("rad: createAtomBiosProxy returned %p", ret);
	return ret;
}

IOReturn RAD::wrapInitializeResources(void *that)
{
	NETDBG::printf("rad: initializeResources called!");
	NETDBG::printf("rad: initializeResources: this = %p", that);
	auto ret = FunctionCast(wrapInitializeResources, callbackRAD->orgInitializeResources)(that);
	NETDBG::printf("rad: initializeResources returned 0x%X", ret);
	return ret;
}

IOReturn RAD::wrapPopulateDeviceMemory(void *that, uint32_t reg)
{
	DBGLOG("rad", "populateDeviceMemory called!");
	DBGLOG("rad", "populateDeviceMemory: this = %p reg = 0x%X", that, reg);
	auto ret = FunctionCast(wrapPopulateDeviceMemory, callbackRAD->orgPopulateDeviceMemory)(that, reg);
	DBGLOG("rad", "populateDeviceMemory returned 0x%X", ret);
	return kIOReturnSuccess;
}

void *RAD::wrapGetGpuHwConstants(uint8_t *param1)
{
	DBGLOG("rad", "----------------------------------------------------------------------");
	DBGLOG("rad", "_GetGpuHwConstants called!");
	DBGLOG("rad", "_GetGpuHwConstants: param1 = %p", param1);
	auto *asicCaps = *(uint8_t **)(param1 + 0x350);
	DBGLOG("rad", "_GetGpuHwConstants: asicCaps = %p", asicCaps);
	uint16_t deviceId = *(uint16_t *)(asicCaps + 8);
	DBGLOG("rad", "_GetGpuHwConstants: deviceId = 0x%X", deviceId);
	auto *goldenSettings = *(uint8_t **)(asicCaps + 48);
	DBGLOG("rad", "_GetGpuHwConstants: goldenSettings = %p", goldenSettings);
	for (size_t i = 0; i < 24; i++) {
		DBGLOG("rad", "_GetGpuHwConstants: goldenSettings: %zu:0x%X", i, goldenSettings[i]);
	}
	auto ret = FunctionCast(wrapGetGpuHwConstants, callbackRAD->orgGetGpuHwConstants)(param1);
	DBGLOG("rad", "_GetGpuHwConstants returned %p", ret);
	DBGLOG("rad", "----------------------------------------------------------------------");
	if (!ret)
	{
		NETDBG::printf("rad: _GetGpuHwConstants failed!");
		NETDBG::printf("_GetGpuHwConstants returned ZERO value!");
		panic("_GetGpuHwConstants returned ZERO value!");
	}
	return ret;
}

uint64_t RAD::wrapMCILUpdateGfxCGPG(void *param1)
{
	
	NETDBG::printf("rad: ----------------------------------------------------------------------");
	NETDBG::printf("_Cail_MCILUpdateGfxCGPG called!");
	NETDBG::printf("rad: _Cail_MCILUpdateGfxCGPG called!");
	NETDBG::printf("rad: _Cail_MCILUpdateGfxCGPG: param1 = %p", param1);
	auto ret = FunctionCast(wrapMCILUpdateGfxCGPG, callbackRAD->orgMCILUpdateGfxCGPG)(param1);
	NETDBG::printf("rad: _Cail_MCILUpdateGfxCGPG returned 0x%llX", ret);
	NETDBG::printf("_Cail_MCILUpdateGfxCGPG returned 0x%llX", ret);
	return ret;
}

IOReturn RAD::wrapQueryEngineRunningState(void *that, void *param1, void *param2)
{
	NETDBG::printf("queryEngineRunningState called!");
	NETDBG::printf("queryEngineRunningState: this = %p param1 = %p param2 = %p", that, param1, param2);
	NETDBG::printf("queryEngineRunningState: *param2 = 0x%X", *static_cast<uint32_t *>(param2));
	auto ret = FunctionCast(wrapQueryEngineRunningState, callbackRAD->orgQueryEngineRunningState)(that, param1, param2);
	NETDBG::printf("queryEngineRunningState: after *param2 = 0x%X", *static_cast<uint32_t *>(param2));
	NETDBG::printf("queryEngineRunningState returned 0x%X", ret);
	return ret;
}

IOReturn RAD::wrapQueryComputeQueueIsIdle(void *that, uint64_t param1)
{
	NETDBG::printf("QueryComputeQueueIsIdle called!");
	NETDBG::printf("QueryComputeQueueIsIdle: this = %p param1 = 0x%llX", that, param1);
	auto ret = FunctionCast(wrapQueryComputeQueueIsIdle, callbackRAD->orgQueryComputeQueueIsIdle)(that, param1);
	NETDBG::printf("QueryComputeQueueIsIdle returned 0x%X", ret);
	return ret;
}

uint64_t RAD::wrapCAILQueryEngineRunningState(void *param1, uint32_t *param2, uint64_t param3)
{
	NETDBG::printf("_CAILQueryEngineRunningState called!");
	NETDBG::printf("_CAILQueryEngineRunningState: param1 = %p param2 = %p param3 = %llX", param1, param2, param3);
	NETDBG::printf("_CAILQueryEngineRunningState: *param2 = 0x%X", *param2);
	auto ret = FunctionCast(wrapCAILQueryEngineRunningState, callbackRAD->orgCAILQueryEngineRunningState)(param1, param2, param3);
	NETDBG::printf("_CAILQueryEngineRunningState: after *param2 = 0x%X", *param2);
	NETDBG::printf("_CAILQueryEngineRunningState returned 0x%llX", ret);
	return ret;
}

uint64_t RAD::wrapCailMonitorEngineInternalState(void *that, uint32_t param1, uint32_t *param2)
{
	NETDBG::printf("_CailMonitorEngineInternalState called!");
	NETDBG::printf("_CailMonitorEngineInternalState: this = %p param1 = 0x%X param2 = %p", that, param1, param2);
	NETDBG::printf("_CailMonitorEngineInternalState: *param2 = 0x%X", *param2);
	auto ret = FunctionCast(wrapCailMonitorEngineInternalState, callbackRAD->orgCailMonitorEngineInternalState)(that, param1, param2);
	NETDBG::printf("_CailMonitorEngineInternalState: after *param2 = 0x%X", *param2);
	NETDBG::printf("_CailMonitorEngineInternalState returned 0x%llX", ret);
	return ret;
}

uint64_t RAD::wrapCailMonitorPerformanceCounter(void *that, uint32_t *param1)
{
	NETDBG::printf("_CailMonitorPerformanceCounter called!");
	NETDBG::printf("_CailMonitorPerformanceCounter: this = %p param1 = %p", that, param1);
	NETDBG::printf("_CailMonitorPerformanceCounter: *param1 = 0x%X", *param1);
	auto ret = FunctionCast(wrapCailMonitorPerformanceCounter, callbackRAD->orgCailMonitorPerformanceCounter)(that, param1);
	NETDBG::printf("_CailMonitorPerformanceCounter: after *param1 = 0x%X", *param1);
	NETDBG::printf("_CailMonitorPerformanceCounter returned 0x%llX", ret);
	return ret;
}

bool RAD::wrapAMDHWChannelWaitForIdle(void *that, uint64_t param1)
{
	NETDBG::printf("AMDRadeonX5000_AMDHWChannel::waitForIdle called!");
	NETDBG::printf("AMDRadeonX5000_AMDHWChannel::waitForIdle: this = %p param1 = 0x%llx", that, param1);
	auto ret = FunctionCast(wrapAMDHWChannelWaitForIdle, callbackRAD->orgAMDHWChannelWaitForIdle)(that, param1);
	NETDBG::printf("AMDRadeonX5000_AMDHWChannel::waitForIdle returned %d", ret);
	return ret;
}

WRAP_SIMPLE(uint64_t, AcceleratorPowerUpHw, "0x%llX")

IOReturn RAD::wrapInitializePP(void* that)
{
	NETDBG::printf("rad: initializePowerPlay called!");
	auto ret = FunctionCast(wrapInitializePP, callbackRAD->orgInitializePP)(that);
	NETDBG::printf("rad: initializePowerPlay returned 0x%X", ret);
	return ret;
}

IOReturn RAD::wrapCreatePowerPlayInterface(void* that)
{
	NETDBG::printf("rad: createPowerPlayInterface called!");
	auto ret = FunctionCast(wrapCreatePowerPlayInterface, callbackRAD->orgCreatePowerPlayInterface)(that);
	NETDBG::printf("rad: createPowerPlayInterface returned 0x%X", ret);
	return ret;
}

void RAD::wrapPPLog(char *param1, [[maybe_unused]] char param2, [[maybe_unused]] char param3, [[maybe_unused]] char param4, [[maybe_unused]] char param5, [[maybe_unused]] char param6, char *param7)
{
	NETDBG::printf("rad: _PP_Log: from %s: %s", param1, param7);
}

IOReturn RAD::wrapSendRequestToAccelerator(void *that, uint32_t param1, void *param2, void *param3, void *param4)
{
	NETDBG::printf("rad: sendRequestToAccelerator called!");
	NETDBG::printf("rad: sendRequestToAccelerator: that = %p param1 = 0x%X param2 = %p param3 = %p param4 = %p", that, param1, param2, param3, param4);
	auto ret = FunctionCast(wrapSendRequestToAccelerator, callbackRAD->orgSendRequestToAccelerator)(that, param1, param2, param3, param4);
	NETDBG::printf("rad: sendRequestToAccelerator returned 0x%X", ret);
	return ret;
}

WRAP_SIMPLE(IOReturn, PPInitialize, "0x%X")

IOReturn RAD::wrapPpEnable(void *that, bool param1)
{
	NETDBG::printf("rad: ppEnable called!");
	NETDBG::printf("rad: ppEnable: that = %p param1 = %d", that, param1);
	auto ret = FunctionCast(wrapPpEnable, callbackRAD->orgPpEnable)(that, param1);
	NETDBG::printf("rad: ppEnable returned 0x%X", ret);
	return ret;
}

WRAP_SIMPLE(IOReturn, UpdatePowerPlay, "0x%X")
WRAP_SIMPLE(bool, IsReady, "%d")

IOReturn RAD::wrapPpDisplayConfigChange(void *that, void *param1, void *param2)
{
	NETDBG::printf("rad: ppDisplayConfigChange called!");
	NETDBG::printf("rad: ppDisplayConfigChange: this = %p param1 = %p param2 = %p", that, param1, param2);
	auto ret = FunctionCast(wrapPpDisplayConfigChange, callbackRAD->orgPpDisplayConfigChange)(that, param1, param2);
	NETDBG::printf("rad: ppDisplayConfigChange returned 0x%X", ret);
	return ret;
}

uint64_t RAD::wrapPECISetupInitInfo(uint32_t *param1, uint32_t *param2)
{
	NETDBG::printf("rad: _PECI_SetupInitInfo called!");
	NETDBG::printf("rad: _PECI_SetupInitInfo: param1 = %p param2 = %p", param1, param2);
	NETDBG::printf("rad: _PECI_SetupInitInfo: *param1 = 0x%X", *param1);
	NETDBG::printf("rad: _PECI_SetupInitInfo: param2 before: 0:0x%X 1:0x%X 2:0x%X 3:0x%X", param2[0], param2[1], param2[2], param2[3]);
	auto ret = FunctionCast(wrapPECISetupInitInfo, callbackRAD->orgPECISetupInitInfo)(param1, param2);
	NETDBG::printf("rad: _PECI_SetupInitInfo: param2 after: 0:0x%X 1:0x%X 2:0x%X 3:0x%X", param2[0], param2[1], param2[2], param2[3]);
	NETDBG::printf("rad: _PECI_SetupInitInfo returned 0x%llX", ret);
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
			{"__ZN13ATIController24sendRequestToAcceleratorE25_eAMDAccelIOFBRequestTypePvS1_S1_", wrapSendRequestToAccelerator, orgSendRequestToAccelerator},
		};
		if (!patcher.routeMultipleLong(index, requests, arrsize(requests), address, size))
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
			{"_IpiSetFwEntry", wrapIpiSetFwEntry, orgIpiSetFwEntry},
			{"_ipi_smu_sw_init", wrapIpiSmuSwInit, orgIpiSmuSwInit},
			{"_smu_sw_init", wrapSmuSwInit, orgSmuSwInit},
			{"_smu_internal_sw_init", wrapSmuInternalSwInit, orgSmuInternalSwInit},
			{"_smu_get_hw_version", wrapSmuGetHwVersion, orgSmuGetHwVersion},
			{"_psp_sw_init", wrapPspSwInit, orgPspSwInit},
			{"_gc_get_hw_version", wrapGcGetHwVersion, orgGcGetHwVersion},
			{"_internal_cos_read_fw", wrapInternalCosReadFw, orgInternalCosReadFw},
			{"__ZN35AMDRadeonX5000_AMDRadeonHWLibsX500025populateFirmwareDirectoryEv", wrapPopulateFirmwareDirectory, orgPopulateFirmwareDirectory},
			{"_GetGpuHwConstants", wrapGetGpuHwConstants, orgGetGpuHwConstants},
			{"__ZN15AmdCailServices23queryEngineRunningStateEP17CailHwEngineQueueP22CailEngineRunningState", wrapQueryEngineRunningState, orgQueryEngineRunningState},
			{"_CAILQueryEngineRunningState", wrapCAILQueryEngineRunningState, orgCAILQueryEngineRunningState},
			{"_CailMonitorEngineInternalState", wrapCailMonitorEngineInternalState, orgCailMonitorEngineInternalState},
			{"_CailMonitorPerformanceCounter", wrapCailMonitorPerformanceCounter, orgCailMonitorPerformanceCounter},
			{"_PP_Log", wrapPPLog},
			{"__ZN20AtiPowerPlayServices8ppEnableEb", wrapPpEnable, orgPpEnable},
			{"__ZN20AtiPowerPlayServices21ppDisplayConfigChangeEP22PPDisplayConfigurationb", wrapPpDisplayConfigChange, orgPpDisplayConfigChange},
			{"_PECI_SetupInitInfo", wrapPECISetupInitInfo, orgPECISetupInitInfo},
		};
		if (!patcher.routeMultipleLong(index, requests, arrsize(requests), address, size))
			panic("RAD: Failed to route AMDRadeonX5000HWLibs symbols");
		
		return true;
	} else if (kextAMD10000Controller.loadIndex == index) {
		DBGLOG("rad", "Hooking AMD10000Controller");
		
		KernelPatcher::RouteRequest requests[] = {
			{"__ZN18AMD10000Controller23findProjectByPartNumberEP20ControllerProperties", wrapProjectByPartNumber},
			{"__ZN18AMD10000Controller35initializeProjectDependentResourcesEv", wrapInitializeProjectDependentResources, orgInitializeProjectDependentResources},
			{"__ZN18AMD10000Controller21hwInitializeFbMemSizeEv", wrapHwInitializeFbMemSize, orgHwInitializeFbMemSize},
			{"__ZN18AMD10000Controller18hwInitializeFbBaseEv", wrapHwInitializeFbBase, orgHwInitializeFbBase},
			{"__ZN18AMD10000Controller19initializeResourcesEv", wrapInitializeResources, orgInitializeResources},
			{"__ZN18AMD10000Controller19initializePowerPlayEv", wrapInitializePP, orgInitializePP},
			{"__ZN22Vega10PowerPlayManager24createPowerPlayInterfaceEv", wrapCreatePowerPlayInterface, orgCreatePowerPlayInterface},
			{"__ZN22Vega10PowerPlayManager10initializeEv", wrapPPInitialize, orgPPInitialize},
			{"__ZN22Vega10PowerPlayManager7isReadyEv", wrapIsReady, orgIsReady},
			{"__ZN22Vega10PowerPlayManager15updatePowerPlayEv", wrapUpdatePowerPlay, orgUpdatePowerPlay},
		};
		if (!patcher.routeMultipleLong(index, requests, arrsize(requests), address, size))
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
					NETDBG::printf("rad: failed to disable write protection for BITS_PER_COMPONENT");
				}
			}
			bitsPerComponent++;
		}
	}
	else
	{
		NETDBG::printf("rad: failed to find BITS_PER_COMPONENT");
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
		NETDBG::printf("rad: failed to patch RGB mask for 24-bit output");
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
	patcher.routeMultipleLong(kextRadeonSupport.loadIndex, requests, address, size);
}

uint64_t RAD::wrapConfigureDevice(void *that, IOPCIDevice *dev)
{
	NETDBG::printf("rad: configureDevice called!");
	auto ret = FunctionCast(wrapConfigureDevice, callbackRAD->orgConfigureDevice)(that, dev);
	NETDBG::printf("rad: configureDevice returned 0x%llX", ret);
	return ret;
}

IOService *RAD::wrapInitLinkToPeer(void *that, const char *matchCategoryName)
{
	NETDBG::printf("rad: initLinkToPeer called!");
	auto ret = FunctionCast(wrapInitLinkToPeer, callbackRAD->orgInitLinkToPeer)(that, matchCategoryName);
	NETDBG::printf("rad: initLinkToPeer returned %p", ret);
	return ret;
}

WRAP_SIMPLE(uint64_t, CreateHWHandler, "0x%llX")

uint64_t RAD::wrapCreateHWInterface(void *that, IOPCIDevice *dev)
{
	NETDBG::printf("rad: createHWInterface called!");
	auto ret = FunctionCast(wrapCreateHWInterface, callbackRAD->orgCreateHWInterface)(that, dev);
	NETDBG::printf("rad: createHWInterface returned 0x%llX", ret);
	return ret;
}

WRAP_SIMPLE(uint64_t, GetHWMemory, "0x%llX")
WRAP_SIMPLE(uint64_t, GetATIChipConfigBit, "0x%llX")
WRAP_SIMPLE(uint64_t, AllocateAMDHWRegisters, "0x%llX")
WRAP_SIMPLE(bool, SetupCAIL, "%d")
WRAP_SIMPLE(uint64_t, InitializeHWWorkarounds, "0x%llX")
WRAP_SIMPLE(uint64_t, AllocateAMDHWAlignManager, "0x%llX")
WRAP_SIMPLE(bool, MapDoorbellMemory, "%d")

uint64_t RAD::wrapGetState(void* that)
{
	DBGLOG("rad", "getState called!");
	auto ret = FunctionCast(wrapGetState, callbackRAD->orgGetState)(that);
	DBGLOG("rad", "getState returned 0x%llX", ret);
	return ret;
}

bool RAD::wrapInitializeTtl(void *that, void *param1)
{
	NETDBG::printf("rad: initializeTtl called!");
	auto ret = FunctionCast(wrapInitializeTtl, callbackRAD->orgInitializeTtl)(that, param1);
	NETDBG::printf("rad: initializeTtl returned %d", ret);
	return ret;
}

WRAP_SIMPLE(uint64_t, ConfRegBase, "0x%llX")
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
		{"__ZN31AMDRadeonX5000_AMDGFX9PM4Engine23QueryComputeQueueIsIdleE18_eAMD_HW_RING_TYPE", wrapQueryComputeQueueIsIdle, orgQueryComputeQueueIsIdle},
		{"__ZN27AMDRadeonX5000_AMDHWChannel11waitForIdleEj", wrapAMDHWChannelWaitForIdle, orgAMDHWChannelWaitForIdle},
		{"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator9powerUpHWEv", wrapAcceleratorPowerUpHw, orgAcceleratorPowerUpHw},
	};
	if (!patcher.routeMultipleLong(hardware.loadIndex, requests, arrsize(requests), address, size))
	{
		panic("Failed to route X5000 symbols");
	}
	
		// Patch AppleGVA support for non-supported models
	if (forceCodecInfo && getHWInfoProcNames[hwIndex] != nullptr)
	{
		KernelPatcher::RouteRequest request(getHWInfoProcNames[hwIndex], wrapGetHWInfo[hwIndex], orgGetHWInfo[hwIndex]);
		if (!patcher.routeMultipleLong(hardware.loadIndex, &request, 1, address, size))
		{
			panic("Failed to route X5000 symbols for AppleGVA support");
		}
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
			NETDBG::printf("rad: prop merge failed to iterate over properties");
		}
	}
	else
	{
		NETDBG::printf("rad: prop merge failed to get properties");
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
		NETDBG::printf("rad: getConnectorsInfoV1 failed %X or undefined %d", code, props == nullptr);
	
	return code;
}

void RAD::updateConnectorsInfo(void *atomutils, t_getAtomObjectTableForType gettable, IOService *ctrl, RADConnectors::Connector *connectors, uint8_t *sz)
{
	if (atomutils)
	{
		NETDBG::printf("rad: getConnectorsInfo found %u connectors", *sz);
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
				NETDBG::printf("rad: getConnectorsInfo got size override to %u", *sz);
			}
			
			if (consPtr && consSize > 0 && *sz > 0 && RADConnectors::valid(consSize, *sz))
			{
				RADConnectors::copy(connectors, *sz, static_cast<const RADConnectors::Connector *>(consPtr), consSize);
				NETDBG::printf("rad: getConnectorsInfo installed %u connectors", *sz);
				applyPropertyFixes(ctrl, *sz);
			}
			else
			{
				NETDBG::printf("rad: getConnectorsInfo conoverrides have invalid size %u for %u num", consSize, *sz);
			}
		}
		else
		{
			NETDBG::printf("rad: getConnectorsInfo conoverrides have invalid type");
		}
	}
	else
	{
		if (atomutils)
		{
			NETDBG::printf("rad: getConnectorsInfo attempting to autofix connectors");
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
				NETDBG::printf("rad: getConnectorsInfo found different displaypaths %u and connectors %u", displayPathNum, connectorObjectNum);
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
			NETDBG::printf("rad: getConnectorInfo found %u senses in connector-priority", senseNum);
			reprioritiseConnectors(senseList, senseNum, connectors, *sz);
		}
		else
		{
			NETDBG::printf("rad: getConnectorInfo leaving unchaged priority");
		}
	}
	
	NETDBG::printf("rad: getConnectorsInfo resulting %u connectors follow", *sz);
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
			NETDBG::printf("rad: translateAtomConnectorInfoV1 got sense id %02X", sense);
			
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
				NETDBG::printf("rad: translateAtomConnectorInfoV1 checking %04X object id", usSrcObjectID);
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
			NETDBG::printf("rad: translateAtomConnectorInfoV1 failed to detect sense for translated connector");
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
			NETDBG::printf("rad: autocorrectConnectors not encoder %X at %u", displayPaths[i].usGraphicObjIds, i);
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
			NETDBG::printf("rad: autocorrectConnectors failed to detect sense for %u connector", i);
			continue;
		}
		
		NETDBG::printf("rad: autocorrectConnectors found txmit %02X enc %02X sense %02X for %u connector", txmit, enc, sense, i);
		
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
			NETDBG::printf("rad: autocorrectConnector found unsupported connector type %02X", connector);
			return;
		}
		
		auto fixTransmit = [](auto &con, uint8_t idx, uint8_t sense, uint8_t txmit)
		{
			if (con.sense == sense)
			{
				if (con.transmitter != txmit && (con.transmitter & 0xCF) == con.transmitter)
				{
					NETDBG::printf("rad: autocorrectConnector replacing txmit %02X with %02X for %u connector sense %02X",
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
		NETDBG::printf("rad: autocorrectConnector use -raddvi to enable dvi autocorrection");
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
						NETDBG::printf("rad: reprioritiseConnectors setting priority of sense %02X to %u by sense", con.sense, priCount);
						con.priority = priCount++;
						return true;
					}
				}
				else
				{
					if (con.priority == 0 && con.type == typeList[i - senseNum])
					{
						NETDBG::printf("rad: reprioritiseConnectors setting priority of sense %02X to %u by type", con.sense, priCount);
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
		NETDBG::printf("rad: getConnectorsInfoV2 failed %X or undefined %d", code, props == nullptr);
	
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
			NETDBG::printf("rad: translateAtomConnectorInfoV2 got sense id %02X", sense);
			uint8_t txmit = 0, enc = 0;
			if (getTxEnc(info->usGraphicObjIds, txmit, enc))
				callbackRAD->autocorrectConnector(getConnectorID(info->usConnObjectId), getSenseID(info->i2cRecord), txmit, enc, connector, 1);
		}
		else
		{
			NETDBG::printf("rad: translateAtomConnectorInfoV2 failed to detect sense for translated connector");
		}
	}
	
	return code;
}

bool RAD::wrapATIControllerStart(IOService *ctrl, IOService *provider)
{
	NETDBG::printf("rad: starting controller " PRIKADDR, CASTKADDR(current_thread()));
	if (callbackRAD->forceVesaMode)
	{
		NETDBG::printf("rad: disabling video acceleration on request");
		return false;
	}
	
	callbackRAD->currentPropProvider.set(provider);
	bool r = FunctionCast(wrapATIControllerStart, callbackRAD->orgATIControllerStart)(ctrl, provider);
	NETDBG::printf("rad: starting controller done %d " PRIKADDR, r, CASTKADDR(current_thread()));
	callbackRAD->currentPropProvider.erase();
	
	return r;
}

bool RAD::doNotTestVram([[maybe_unused]] IOService * ctrl, [[maybe_unused]] uint32_t reg, [[maybe_unused]] bool retryOnFail)
{
	NETDBG::printf("rad: TestVRAM called! Returning true");
	return true;
}

bool RAD::wrapNotifyLinkChange(void *atiDeviceControl, kAGDCRegisterLinkControlEvent_t event, void *eventData, uint32_t eventFlags)
{
	auto ret = FunctionCast(wrapNotifyLinkChange, callbackRAD->orgNotifyLinkChange)(atiDeviceControl, event, eventData, eventFlags);
	
	if (event == kAGDCValidateDetailedTiming)
	{
		auto cmd = static_cast<AGDCValidateDetailedTiming_t *>(eventData);
		NETDBG::printf("rad: AGDCValidateDetailedTiming %u -> %d (%u)", cmd->framebufferIndex, ret, cmd->modeStatus);
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
		NETDBG::printf("rad: getHWInfo: no parent found for accelVideoCtx!");
		return;
	}
	pciDev = OSDynamicCast(IOService, accel->getParentEntry(gIOServicePlane));
	if (pciDev == NULL)
	{
		NETDBG::printf("rad: getHWInfo: no parent found for accel!");
		return;
	}
	uint16_t &org = getMember<uint16_t>(hwInfo, 0x4);
	uint32_t dev = org;
	if (!WIOKit::getOSDataValue(pciDev, "codec-device-id", dev))
	{
		WIOKit::getOSDataValue(pciDev, "device-id", dev);
	}
	NETDBG::printf("rad: getHWInfo: original PID: 0x%04X, replaced PID: 0x%04X", org, dev);
	org = static_cast<uint16_t>(dev);
}
