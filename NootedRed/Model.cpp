// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_iokit.hpp>
#include <Headers/kern_util.hpp>
#include <PrivateHeaders/Model.hpp>

enum struct MatchType {
    RevOnly,
    RevSubsys,
};

struct Model {
    const MatchType matchType;
    const UInt16 revision;
    const UInt16 subsystemId;
    const UInt16 subsystemVendor;
    const char *const name;

    constexpr Model(const UInt16 revision, const char *const name)
        : matchType {MatchType::RevOnly}, revision {revision}, subsystemId {0}, subsystemVendor {0}, name {name} {}
    constexpr Model(const UInt16 revision, const UInt16 subsystemId, const UInt16 subsystemVendor,
        const char *const name)
        : matchType {MatchType::RevSubsys}, revision {revision}, subsystemId {subsystemId},
          subsystemVendor {subsystemVendor}, name {name} {}
};

struct DevicePair {
    const UInt16 deviceId;
    const Model *const models;
    const size_t count;
    const char *const fallback;

    constexpr DevicePair(const UInt16 deviceId, const Model *const models, const size_t count,
        const char *const fallback)
        : deviceId {deviceId}, models {models}, count {count}, fallback {fallback} {}
    constexpr DevicePair(const UInt16 deviceId, const char *const fallback)
        : DevicePair(deviceId, nullptr, 0, fallback) {}

    template<const size_t N>
    constexpr DevicePair(const UInt16 deviceId, const Model (&models)[N], const char *const fallback)
        : DevicePair(deviceId, models, N, fallback) {}
};

static const Model dev15DD[] = {
    {0x81, "AMD Radeon RX Vega 11"},
    {0x82, "AMD Radeon RX Vega 8"},
    {0x83, "AMD Radeon RX Vega 8"},
    {0x84, "AMD Radeon RX Vega 6"},
    {0x85, "AMD Radeon RX Vega 3"},
    {0x86, "AMD Radeon RX Vega 11"},
    {0x88, "AMD Radeon RX Vega 8"},
    {0xC1, "AMD Radeon RX Vega 11"},
    {0xC2, "AMD Radeon RX Vega 8"},
    {0xC3, "AMD Radeon RX Vega 10"},
    {0xC4, "AMD Radeon RX Vega 8"},
    {0xC5, "AMD Radeon RX Vega 3"},
    {0xC6, "AMD Radeon RX Vega 11"},
    {0xC8, "AMD Radeon RX Vega 8"},
    {0xC9, "AMD Radeon RX Vega 11"},
    {0xCA, "AMD Radeon RX Vega 8"},
    {0xCB, "AMD Radeon RX Vega 3"},
    {0xCC, "AMD Radeon RX Vega 6"},
    {0xCE, "AMD Radeon RX Vega 3"},
    {0xCF, "AMD Radeon RX Vega 3"},
    {0xD0, "AMD Radeon RX Vega 10"},
    {0xD1, "AMD Radeon RX Vega 8"},
    {0xD3, "AMD Radeon RX Vega 11"},
    {0xD5, "AMD Radeon RX Vega 8"},
    {0xD6, "AMD Radeon RX Vega 11"},
    {0xD7, "AMD Radeon RX Vega 8"},
    {0xD8, "AMD Radeon RX Vega 3"},
    {0xD9, "AMD Radeon RX Vega 6"},
    {0xE1, "AMD Radeon Vega 3"},
    {0xE2, "AMD Radeon Vega 3"},
};

static const Model dev15D8[] = {
    {0x00, "AMD Radeon RX Vega 8 WS"},
    {0x91, "AMD Radeon RX Vega 3"},
    {0x92, "AMD Radeon RX Vega 3"},
    {0x93, "AMD Radeon RX Vega 1"},
    {0xA1, "AMD Radeon Vega 10"},
    {0xA2, "AMD Radeon Vega 8"},
    {0xA3, "AMD Radeon Vega 6"},
    {0xA4, "AMD Radeon Vega 3"},
    {0xB1, "AMD Radeon Vega 10"},
    {0xB2, "AMD Radeon Vega 8"},
    {0xB3, "AMD Radeon Vega 6"},
    {0xB4, "AMD Radeon Vega 3"},
    {0xC1, "AMD Radeon RX Vega 10"},
    {0xC2, "AMD Radeon RX Vega 8"},
    {0xC3, "AMD Radeon RX Vega 6"},
    {0xC4, "AMD Radeon RX Vega 3"},
    {0xC5, "AMD Radeon RX Vega 3"},
    {0xC8, "AMD Radeon RX Vega 11"},
    {0xC9, "AMD Radeon RX Vega 8"},
    {0xCA, "AMD Radeon RX Vega 11"},
    {0xCB, "AMD Radeon RX Vega 8"},
    {0xCC, "AMD Radeon RX Vega 3"},
    {0xCE, "AMD Radeon RX Vega 3"},
    {0xCF, "AMD Radeon RX Vega 3"},
    {0xD1, "AMD Radeon RX Vega 10"},
    {0xD2, "AMD Radeon RX Vega 8"},
    {0xD3, "AMD Radeon RX Vega 6"},
    {0xD4, "AMD Radeon RX Vega 3"},
    {0xD8, "AMD Radeon RX Vega 11"},
    {0xD9, "AMD Radeon RX Vega 8"},
    {0xDA, "AMD Radeon RX Vega 11"},
    {0xDB, "AMD Radeon RX Vega 8"},
    {0xDC, "AMD Radeon RX Vega 3"},
    {0xDD, "AMD Radeon RX Vega 3"},
    {0xDE, "AMD Radeon RX Vega 3"},
    {0xDF, "AMD Radeon RX Vega 3"},
    {0xE1, 0x0034, 0x1414, "AMD Radeon RX Vega 11"},
    {0xE2, 0x0034, 0x1414, "AMD Radeon RX Vega 9"},
    {0xE3, "AMD Radeon RX Vega 3"},
    {0xE4, "AMD Radeon RX Vega 3"},
};

static const Model dev1636[] = {
    {0xC2, 0x381C, 0x17AA, "AMD Radeon RX Renoir Graphics C2"},
    {0xC2, 0x507F, 0x17AA, "AMD Radeon RX Renoir Graphics C2"},
    {0xC3, 0x381C, 0x17AA, "AMD Radeon RX Renoir Graphics C3"},
    {0xC3, 0x507F, 0x17AA, "AMD Radeon RX Renoir Graphics C3"},
    {0xC4, 0x381C, 0x17AA, "AMD Radeon RX Renoir Graphics C4"},
    {0xC4, 0x507F, 0x17AA, "AMD Radeon RX Renoir Graphics C4"},
    {0xD1, 0x16CF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x16DF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x16EF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x16FF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x171F, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x172F, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x17DF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x17EF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x18BF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x18CF, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x1E11, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x1E21, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x1F11, 0x1043, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x3813, 0x17AA, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x381B, 0x17AA, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x381C, 0x17AA, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x3F1A, 0x17AA, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x507F, 0x17AA, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x5081, 0x17AA, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x5082, 0x17AA, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x5099, 0x17AA, "AMD Radeon Pro Graphics"},
    {0xD1, 0x86EE, 0x103C, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x86F1, 0x103C, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x8786, 0x103C, "AMD Radeon RX Renoir Graphics D1"},
    {0xD1, 0x8788, 0x103C, "AMD Radeon RX Renoir Graphics D1"},
    {0xD3, 0x5099, 0x17AA, "AMD Radeon Pro Graphics"},
    {0xD4, 0x16CF, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x16DF, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x16EF, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x16FF, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x171F, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x172F, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x17DF, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x17EF, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x1E11, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x1E21, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x1F11, 0x1043, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x380D, 0x17AA, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x381B, 0x17AA, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x381C, 0x17AA, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x507F, 0x17AA, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x86EE, 0x103C, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x86F1, 0x103C, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x8786, 0x103C, "AMD Radeon RX Renoir Graphics D4"},
    {0xD4, 0x8788, 0x103C, "AMD Radeon RX Renoir Graphics D4"},
};

static const DevicePair devices[] = {
    {0x15DD, dev15DD, "AMD Radeon RX Graphics"},
    {0x15D8, dev15D8, "AMD Radeon RX Graphics"},
    {0x15E7, "AMD Radeon RX Graphics"},
    {0x1636, dev1636, "AMD Radeon RX Graphics"},
    {0x1638, "AMD Radeon RX Graphics"},
    {0x164C, "AMD Radeon RX Graphics"},
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
