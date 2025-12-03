// AMD CAIL Result
//
// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once

enum CAILResult {
    kCAILResultOK = 0,
    kCAILResultFailed,
    kCAILResultInvalidParameters,
    kCAILResultNoResponse,
    kCAILResultUnsupported,
    kCAILResultCatastrophic,
    kCAILResultMCRangeNotMapped = 122,
    kCAILResultPowerControlRefused = 160,
};
