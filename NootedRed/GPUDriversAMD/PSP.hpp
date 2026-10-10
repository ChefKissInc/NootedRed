// AMD GPU Platform Security Processor
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

enum AMDPSPCommand
{
    kPSPCommandLoadTA   = 1,
    kPSPCommandLoadASD  = 4,
    kPSPCommandLoadIPFW = 6,
};

enum AMDPSPFirmwareID
{
    kPSPFirmwareUnknown,
    kPSPFirmwareSMU,
    kPSPFirmwareCE,
    kPSPFirmwarePFP,
    kPSPFirmwareME,
    kPSPFirmwareMEC1JT,
    kPSPFirmwareMEC2JT,
    kPSPFirmwareMEC1,
    kPSPFirmwareMEC2,
    kPSPFirmwareMES,
    kPSPFirmwareMESStack,
    kPSPFirmwareRLC,
    kPSPFirmwareSDMA0,
    kPSPFirmwareSDMA1,
    kPSPFirmwareVCN0,
    kPSPFirmwareUVD,
    kPSPFirmwareUVD1,
    kPSPFirmwareVCE,
    kPSPFirmwareISP,
    kPSPFirmwareDMCUERAM,
    kPSPFirmwareDMCUISR,
    kPSPFirmwareRLCV,
    kPSPFirmwareRLCP,
    kPSPFirmwareRLCSRListGPM,
    kPSPFirmwareRLCSRListSRM,
    kPSPFirmwareRLCSRListCntl,
    kPSPFirmwareRLCLX6IRAM,
    kPSPFirmwareRLCLX6DRAM,
    kPSPFirmwareRLCVCN0DRAM,
    kPSPFirmwareRLCVCN1DRAM,
    kPSPFirmwareGlobalTapDelays,
    kPSPFirmwareSE0TapDelays,
    kPSPFirmwareSE1TapDelays,
    kPSPFirmwareSE2TapDelays,
    kPSPFirmwareSE3TapDelays,
    kPSPFirmwareDMCUB,
    kPSPFirmwareSDMA2,
    kPSPFirmwareSDMA3,
    kPSPFirmwareSDMA4,
    kPSPFirmwareSDMA5,
    kPSPFirmwareSDMA6,
    kPSPFirmwareSDMA7,
    kPSPFirmwareVCN1,
    kPSPFirmwareDriverCap,
    kPSPFirmwareIMUInstruction,
    kPSPFirmwareIMUData,
    kPSPFirmwareSDMAThread0,
    kPSPFirmwareSDMAThread1,
    kPSPFirmwarePPTable,
    kPSPFirmwareRS64MES,
    kPSPFirmwareRS64MESStack,
    kPSPFirmwareRS64KIQ,
    kPSPFirmwareRS64KIQStack,
};
