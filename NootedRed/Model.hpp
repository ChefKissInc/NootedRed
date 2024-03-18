//! Copyright © 2022-2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

struct Model {
    UInt16 revision {0};
    const char *name {nullptr};
};

struct DevicePair {
    UInt16 deviceId;
    const Model *models;
    size_t count;
};

static constexpr Model dev15DD[] = {
    {0x81, "AMD Radeon Vega 11"},
    {0x82, "AMD Radeon Vega 8"},
    {0x83, "AMD Radeon Vega 8"},
    {0x84, "AMD Radeon Vega 6"},
    {0x85, "AMD Radeon Vega 3"},
    {0x86, "AMD Radeon Vega 11"},
    {0x88, "AMD Radeon Vega 8"},
    {0xC1, "AMD Radeon Vega 11"},
    {0xC2, "AMD Radeon Vega 8"},
    {0xC3, "AMD Radeon Vega 10"},
    {0xC4, "AMD Radeon Vega 8"},
    {0xC5, "AMD Radeon Vega 3"},
    {0xC6, "AMD Radeon Vega 11"},
    {0xC8, "AMD Radeon Vega 8"},
    {0xC9, "AMD Radeon Vega 11"},
    {0xCA, "AMD Radeon Vega 8"},
    {0xCB, "AMD Radeon Vega 3"},
    {0xCC, "AMD Radeon Vega 6"},
    {0xCE, "AMD Radeon Vega 3"},
    {0xCF, "AMD Radeon Vega 3"},
    {0xD0, "AMD Radeon Vega 10"},
    {0xD1, "AMD Radeon Vega 8"},
    {0xD3, "AMD Radeon Vega 11"},
    {0xD5, "AMD Radeon Vega 8"},
    {0xD6, "AMD Radeon Vega 11"},
    {0xD7, "AMD Radeon Vega 8"},
    {0xD8, "AMD Radeon Vega 3"},
    {0xD9, "AMD Radeon Vega 6"},
    {0xE1, "AMD Radeon Vega 3"},
    {0xE2, "AMD Radeon Vega 3"},
};

static constexpr Model dev15D8[] = {
    {0x00, "AMD Radeon Vega 8 WS"},
    {0x91, "AMD Radeon Vega 3"},
    {0x92, "AMD Radeon Vega 3"},
    {0x93, "AMD Radeon Vega 1"},
    {0xA1, "AMD Radeon Vega 10"},
    {0xA2, "AMD Radeon Vega 8"},
    {0xA3, "AMD Radeon Vega 6"},
    {0xA4, "AMD Radeon Vega 3"},
    {0xB1, "AMD Radeon Vega 10"},
    {0xB2, "AMD Radeon Vega 8"},
    {0xB3, "AMD Radeon Vega 6"},
    {0xB4, "AMD Radeon Vega 3"},
    {0xC1, "AMD Radeon Vega 10"},
    {0xC2, "AMD Radeon Vega 8"},
    {0xC3, "AMD Radeon Vega 6"},
    {0xC4, "AMD Radeon Vega 3"},
    {0xC5, "AMD Radeon Vega 3"},
    {0xC8, "AMD Radeon Vega 11"},
    {0xC9, "AMD Radeon Vega 8"},
    {0xCA, "AMD Radeon Vega 11"},
    {0xCB, "AMD Radeon Vega 8"},
    {0xCC, "AMD Radeon Vega 3"},
    {0xCE, "AMD Radeon Vega 3"},
    {0xCF, "AMD Radeon Vega 3"},
    {0xD1, "AMD Radeon Vega 10"},
    {0xD2, "AMD Radeon Vega 8"},
    {0xD3, "AMD Radeon Vega 6"},
    {0xD4, "AMD Radeon Vega 3"},
    {0xD8, "AMD Radeon Vega 11"},
    {0xD9, "AMD Radeon Vega 8"},
    {0xDA, "AMD Radeon Vega 11"},
    {0xDB, "AMD Radeon Vega 3"},
    {0xDC, "AMD Radeon Vega 3"},
    {0xDD, "AMD Radeon Vega 3"},
    {0xDE, "AMD Radeon Vega 3"},
    {0xDF, "AMD Radeon Vega 3"},
    {0xE1, "AMD Radeon Vega 11"},
    {0xE2, "AMD Radeon Vega 9"},
    {0xE3, "AMD Radeon Vega 3"},
    {0xE4, "AMD Radeon Vega 3"},
};

static constexpr DevicePair devices[] = {
    {0x15DD, dev15DD, arrsize(dev15DD)},
    {0x15D8, dev15D8, arrsize(dev15D8)},
};

inline const char *getBranding(UInt16 dev, UInt16 rev) {
    for (auto &device : devices) {
        if (device.deviceId == dev) {
            for (size_t i = 0; i < device.count; i++) {
                auto &model = device.models[i];
                if (model.revision == rev) { return model.name; }
            }
            break;
        }
    }

    return "AMD Radeon Graphics";
}
