//
//  kern_start.cpp
//  WhateverRed
//
//  Copyright © 2017 vit9696. All rights reserved.
//  Copyright © 2022 VisualDevelopment. All rights reserved.
//

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>

#include "kern_wred.hpp"

static WRed wred;

static const char *bootargOff[] = {
	"-wredoff",
};

static const char *bootargDebug[] = {
	"-wreddbg",
};

static const char *bootargBeta[] = {
	"-wredbeta",
};

PluginConfiguration ADDPR(config){
	xStringify(PRODUCT_NAME),
	parseModuleVersion(xStringify(MODULE_VERSION)),
	LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
	bootargOff,
	arrsize(bootargOff),
	bootargDebug,
	arrsize(bootargDebug),
	bootargBeta,
	arrsize(bootargBeta),
	KernelVersion::SnowLeopard,
	KernelVersion::Monterey,
	[]()
	{
		wred.init();
	},
};
