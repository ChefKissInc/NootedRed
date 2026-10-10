// AMD PowerPlay
//
// Copyright (C) 2024-2026  ChefKiss.
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
#include <IOKit/IOTypes.h>

enum struct PPLogLevel
{
    Error = 1,
    Warning,
    Info,
    MoreInfo,
    Trace,
};

enum struct PPResult
{
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
    PPResultTimeOut,
};

static constexpr UInt32 PP_WAIT_ON_REGISTER_TIMEOUT_DEFAULT = 2000;
