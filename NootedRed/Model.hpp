// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/pci/IOPCIDevice.h>

const char *getBrandingNameForDev(IOPCIDevice *device);
