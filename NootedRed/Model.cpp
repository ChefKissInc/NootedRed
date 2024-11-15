// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_iokit.hpp>
#include <Headers/kern_util.hpp>
#include <PrivateHeaders/Model.hpp>

enum struct MatchType {
    RevOnly,
    RevSubsys,
};

struct Model {
    MatchType matchType {MatchType::RevOnly};
    UInt16 revision {0};
    UInt16 subsystemId {0};
    UInt16 subsystemVendor {0};
    const char *name {nullptr};
};

struct DevicePair {
    UInt16 deviceId;
    const Model *models;
    size_t count;
    const char *fallback;
};

static const Model dev15DD[] = {
    {MatchType::RevOnly, 0xC1, 0x0000, 0x0000, "AMD Radeon RX Vega 11"},
    {MatchType::RevOnly, 0xC2, 0x0000, 0x0000, "AMD Radeon RX Vega 8"},
    {MatchType::RevOnly, 0xC3, 0x0000, 0x0000, "AMD Radeon RX Vega 10"},
    {MatchType::RevOnly, 0xC4, 0x0000, 0x0000, "AMD Radeon RX Vega 8"},
    {MatchType::RevOnly, 0xC5, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xC6, 0x0000, 0x0000, "AMD Radeon RX Vega 11"},
    {MatchType::RevSubsys, 0xC6, 0xD000, 0x1458, "AMD Radeon RX Vega 11"},
    {MatchType::RevOnly, 0xC8, 0x0000, 0x0000, "AMD Radeon RX Vega 8"},
    {MatchType::RevSubsys, 0xC8, 0xD000, 0x1458, "AMD Radeon RX Vega 8"},
    {MatchType::RevOnly, 0xC9, 0x0000, 0x0000, "AMD Radeon RX Vega 11"},
    {MatchType::RevSubsys, 0xC9, 0xD000, 0x1458, "AMD Radeon RX Vega 11"},
    {MatchType::RevOnly, 0xCA, 0x0000, 0x0000, "AMD Radeon RX Vega 8"},
    {MatchType::RevSubsys, 0xCA, 0xD000, 0x1458, "AMD Radeon RX Vega 8"},
    {MatchType::RevOnly, 0xCB, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevSubsys, 0xCB, 0x876B, 0x1043, "AMD Radeon RX Vega 3"},
    {MatchType::RevSubsys, 0xCB, 0xD000, 0x1458, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xCC, 0x0000, 0x0000, "AMD Radeon RX Vega 6"},
    {MatchType::RevOnly, 0xCE, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xCF, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xD0, 0x0000, 0x0000, "AMD Radeon RX Vega 10"},
    {MatchType::RevOnly, 0xD1, 0x0000, 0x0000, "AMD Radeon RX Vega 8"},
    {MatchType::RevOnly, 0xD3, 0x0000, 0x0000, "AMD Radeon RX Vega 11"},
    {MatchType::RevSubsys, 0xD3, 0xD000, 0x1458, "AMD Radeon RX Vega 11"},
    {MatchType::RevOnly, 0xD5, 0x0000, 0x0000, "AMD Radeon RX Vega 8"},
    {MatchType::RevSubsys, 0xD5, 0xD000, 0x1458, "AMD Radeon RX Vega 8"},
    {MatchType::RevOnly, 0xD6, 0x0000, 0x0000, "AMD Radeon RX Vega 11"},
    {MatchType::RevSubsys, 0xD6, 0xD000, 0x1458, "AMD Radeon RX Vega 11"},
    {MatchType::RevOnly, 0xD7, 0x0000, 0x0000, "AMD Radeon RX Vega 8"},
    {MatchType::RevSubsys, 0xD7, 0xD000, 0x1458, "AMD Radeon RX Vega 8"},
    {MatchType::RevOnly, 0xD8, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevSubsys, 0xD8, 0xD000, 0x1458, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xD9, 0x0000, 0x0000, "AMD Radeon RX Vega 6"},
};

static const Model dev15D8[] = {
    {MatchType::RevOnly, 0x91, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0x92, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xC1, 0x0000, 0x0000, "AMD Radeon RX Vega 10"},
    {MatchType::RevOnly, 0xC2, 0x0000, 0x0000, "AMD Radeon RX Vega 8"},
    {MatchType::RevOnly, 0xC3, 0x0000, 0x0000, "AMD Radeon RX Vega 6"},
    {MatchType::RevOnly, 0xC4, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xC5, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xC8, 0x0000, 0x0000, "AMD Radeon RX Vega 11"},
    {MatchType::RevSubsys, 0xC8, 0xD000, 0x1458, "AMD Radeon RX Vega 11"},
    {MatchType::RevSubsys, 0xC8, 0xD001, 0x1458, "AMD Radeon RX Vega 11"},
    {MatchType::RevOnly, 0xC9, 0x0000, 0x0000, "AMD Radeon RX Vega 8"},
    {MatchType::RevSubsys, 0xC9, 0xD000, 0x1458, "AMD Radeon RX Vega 8"},
    {MatchType::RevSubsys, 0xC9, 0xD001, 0x1458, "AMD Radeon RX Vega 8"},
    {MatchType::RevOnly, 0xCA, 0x0000, 0x0000, "AMD Radeon RX Vega 11"},
    {MatchType::RevSubsys, 0xCA, 0xD000, 0x1458, "AMD Radeon RX Vega 11"},
    {MatchType::RevOnly, 0xCB, 0x0000, 0x0000, "AMD Radeon RX Vega 8"},
    {MatchType::RevSubsys, 0xCB, 0xD000, 0x1458, "AMD Radeon RX Vega 8"},
    {MatchType::RevOnly, 0xCC, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevSubsys, 0xCC, 0xD000, 0x1458, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xCE, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xCF, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xD1, 0x0000, 0x0000, "AMD Radeon RX Vega 10"},
    {MatchType::RevOnly, 0xD2, 0x0000, 0x0000, "AMD Radeon RX Vega 8"},
    {MatchType::RevOnly, 0xD3, 0x0000, 0x0000, "AMD Radeon RX Vega 6"},
    {MatchType::RevOnly, 0xD4, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xD8, 0x0000, 0x0000, "AMD Radeon RX Vega 11"},
    {MatchType::RevSubsys, 0xD8, 0xD000, 0x1458, "AMD Radeon RX Vega 11"},
    {MatchType::RevOnly, 0xD9, 0x0000, 0x0000, "AMD Radeon RX Vega 8"},
    {MatchType::RevSubsys, 0xD9, 0xD000, 0x1458, "AMD Radeon RX Vega 8"},
    {MatchType::RevOnly, 0xDA, 0x0000, 0x0000, "AMD Radeon RX Vega 11"},
    {MatchType::RevSubsys, 0xDA, 0xD000, 0x1458, "AMD Radeon RX Vega 11"},
    {MatchType::RevOnly, 0xDB, 0x0000, 0x0000, "AMD Radeon RX Vega 8"},
    {MatchType::RevSubsys, 0xDB, 0xD000, 0x1458, "AMD Radeon RX Vega 8"},
    {MatchType::RevOnly, 0xDC, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevSubsys, 0xDC, 0xD000, 0x1458, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xDD, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xDE, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevOnly, 0xDF, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
    {MatchType::RevSubsys, 0xE1, 0x0034, 0x1414, "AMD Radeon RX Vega 11"},
    {MatchType::RevSubsys, 0xE2, 0x0034, 0x1414, "AMD Radeon RX Vega 9"},
    {MatchType::RevOnly, 0xE4, 0x0000, 0x0000, "AMD Radeon RX Vega 3"},
};

static const Model dev1636[] = {
    {MatchType::RevSubsys, 0xC2, 0x381C, 0x17AA, "AMD Radeon RX Renoir Graphics C2"},
    {MatchType::RevSubsys, 0xC2, 0x507F, 0x17AA, "AMD Radeon RX Renoir Graphics C2"},
    {MatchType::RevSubsys, 0xC3, 0x381C, 0x17AA, "AMD Radeon RX Renoir Graphics C3"},
    {MatchType::RevSubsys, 0xC3, 0x507F, 0x17AA, "AMD Radeon RX Renoir Graphics C3"},
    {MatchType::RevSubsys, 0xC4, 0x381C, 0x17AA, "AMD Radeon RX Renoir Graphics C4"},
    {MatchType::RevSubsys, 0xC4, 0x507F, 0x17AA, "AMD Radeon RX Renoir Graphics C4"},
    {MatchType::RevSubsys, 0xD1, 0x16CF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x16DF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x16EF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x16FF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x171F, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x172F, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x17DF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x17EF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x18BF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x18CF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x1E11, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x1E21, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x1F11, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x3813, 0x17AA, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x381B, 0x17AA, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x381C, 0x17AA, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x3F1A, 0x17AA, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x507F, 0x17AA, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x5081, 0x17AA, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x5082, 0x17AA, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x5099, 0x17AA, "AMD Radeon Pro Graphics"},
    {MatchType::RevSubsys, 0xD1, 0x86EE, 0x103C, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x86F1, 0x103C, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x8786, 0x103C, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD1, 0x8788, 0x103C, "AMD Radeon RX Renoir Graphics D1"},
    {MatchType::RevSubsys, 0xD3, 0x5099, 0x17AA, "AMD Radeon Pro Graphics"},
    {MatchType::RevSubsys, 0xD4, 0x16CF, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x16DF, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x16EF, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x16FF, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x171F, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x172F, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x17DF, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x17EF, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x1E11, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x1E21, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x1F11, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x380D, 0x17AA, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x381B, 0x17AA, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x381C, 0x17AA, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x507F, 0x17AA, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x86EE, 0x103C, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x86F1, 0x103C, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x8786, 0x103C, "AMD Radeon RX Renoir Graphics D4"},
    {MatchType::RevSubsys, 0xD4, 0x8788, 0x103C, "AMD Radeon RX Renoir Graphics D4"},
};

static const DevicePair devices[] = {
    {0x15DD, dev15DD, arrsize(dev15DD), "AMD Radeon RX Graphics"},
    {0x15D8, dev15D8, arrsize(dev15D8), "AMD Radeon RX Graphics"},
    {0x15E7, nullptr, 0, "AMD Radeon RX Graphics"},
    {0x1636, dev1636, arrsize(dev1636), "AMD Radeon RX Graphics"},
    {0x1638, nullptr, 0, "AMD Radeon RX Graphics"},
    {0x164C, nullptr, 0, "AMD Radeon RX Graphics"},
};

const char *getBrandingNameForDev(IOPCIDevice *device) {
    auto deviceId = WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigDeviceID);
    auto revisionId = WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigRevisionID);
    auto subsystemId = WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigSubSystemID);
    auto subsystemVendor = WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigSubSystemVendorID);
    for (auto &devicePair : devices) {
        if (devicePair.deviceId == deviceId) {
            for (size_t i = 0; i < devicePair.count; i++) {
                auto &model = devicePair.models[i];
                if (model.revision != revisionId ||
                    (model.matchType == MatchType::RevSubsys &&
                        (model.subsystemId != subsystemId || model.subsystemVendor != subsystemVendor))) {
                    continue;
                }

                return model.name;
            }

            return devicePair.fallback;
        }
    }

    return nullptr;
}
