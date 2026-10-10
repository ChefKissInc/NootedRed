// AMD CAIL HW Block definitions
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

// No VPE, LSDMA, XGMI?
enum CAILHWBlock
{
    kCAILHWBlockUnknown = 0,
    // 1, 2, 3 = ??? Maybe VPE, LSDMA, XGMI? get_hw_block_type doesn't seem to reference them
    kCAILHWBlockMP1 = 4,
    kCAILHWBlockMP2,
    kCAILHWBlockTHM,
    kCAILHWBlockSMUIO,
    kCAILHWBlockFUSE,
    kCAILHWBlockCLKA,
    kCAILHWBlockPWR,
    kCAILHWBlockGC,
    kCAILHWBlockUVD,
    kCAILHWBlockVCN = kCAILHWBlockUVD,
    kCAILHWBlockAUDIO_AZ,
    kCAILHWBlockACP,
    kCAILHWBlockDCI,
    kCAILHWBlockDMU,
    kCAILHWBlockDCO,
    kCAILHWBlockDIO,
    kCAILHWBlockXDMA,
    kCAILHWBlockDCEAZ,
    kCAILHWBlockDAZ,
    kCAILHWBlockSDPMUX,
    kCAILHWBlockNTB,
    kCAILHWBlockIOHC,
    kCAILHWBlockL2IMU,
    kCAILHWBlockVCE,
    kCAILHWBlockMMHUB,
    kCAILHWBlockATHUB,
    kCAILHWBlockDBGU_NBIO,
    kCAILHWBlockDFX,
    kCAILHWBlockDBGU0,
    kCAILHWBlockDBGU1,
    kCAILHWBlockOSSSYS,
    kCAILHWBlockHDP,
    kCAILHWBlockSDMA0,
    kCAILHWBlockSDMA1,
    kCAILHWBlockISP,
    kCAILHWBlockDBGU_IO,
    kCAILHWBlockDF,
    kCAILHWBlockCLKB,
    kCAILHWBlockFCH,
    kCAILHWBlockDFX_DAP,
    kCAILHWBlockL1IMU_PCIE,
    kCAILHWBlockL1IMU_NBIF,
    kCAILHWBlockL1IMU_IOAGR,
    kCAILHWBlockL1IMU3,
    kCAILHWBlockL1IMU4,
    kCAILHWBlockL1IMU5,
    kCAILHWBlockL1IMU6,
    kCAILHWBlockL1IMU7,
    kCAILHWBlockL1IMU8,
    kCAILHWBlockL1IMU9,
    kCAILHWBlockL1IMU10,
    kCAILHWBlockL1IMU11,
    kCAILHWBlockL1IMU12,
    kCAILHWBlockL1IMU13,
    kCAILHWBlockL1IMU14,
    kCAILHWBlockL1IMU15,
    kCAILHWBlockWAFLC,
    kCAILHWBlockFCH_USB_PD,
    kCAILHWBlockPCIE,
    kCAILHWBlockPCS,
    kCAILHWBlockDDCL,
    kCAILHWBlockSST,
    kCAILHWBlockIOAGR,
    kCAILHWBlockNBIF,
    kCAILHWBlockIOAPIC,
    kCAILHWBlockSYSTEMHUB,
    kCAILHWBlockNTBCCP,
    kCAILHWBlockUMC,
    kCAILHWBlockSATA,
    kCAILHWBlockUSB,
    kCAILHWBlockCCXSEC,
    kCAILHWBlockXGBE,
    kCAILHWBlockMP0,
    kCAILHWBlockSDMA2,
    kCAILHWBlockSDMA3,
};
