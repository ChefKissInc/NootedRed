// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

enum PPResult {
    PPResultOK = 1,
    PPResultFailed,
    PPResultNotSupported,
    PPResultNotInitialized,
    PPResultBadInputSize,
    PPResultBadOutputSize,
    PPResultBadInput,
    PPResultBadOutput,
    PPResultInitializationFailed,
    PPResultAsicNotSupported,
    PPResultVBIOSCorrupt,
    PPResultOutOfMemory,
    PPResultNoThermalController,
    PPResultThermalControllerNoFan,
    PPResultStateNotFound,
    PPResultStateNotComparable,
    PPResultStateIsCurrent,
    PPResultStateIsRequested,
    PPResultTableImmediateExit,
    PPResultBiosTableNotFound,
    PPResultPXSwitchDeferred,
    PPResultRetry,
    PPResultTooManyInstances,
    PPResultOutOfRange,
    PPResultInSurpriseRemoval,
    PPResultTableImmediateExitOK,
    PPResultClientNotFound,
    PPResultTimeOut
};
static_assert(sizeof(PPResult) == 0x4);
