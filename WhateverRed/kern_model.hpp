//  Copyright © 2022 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_util.hpp>

struct Model {
    uint16_t rev {0};
    const char *name {nullptr};
};

struct DevicePair {
    uint16_t dev;
    const Model *models;
    size_t modelNum;
};

static constexpr Model dev15d8[] = {
    {0x00, "AMD Radeon RX Vega 8 Graphics WS"},
    {0x91, "AMD Radeon Vega 3 Graphics"},
    {0x91, "AMD Ryzen Embedded R1606G with Radeon Vega Gfx"},
    {0x92, "AMD Radeon Vega 3 Graphics"},
    {0x92, "AMD Ryzen Embedded R1505G with Radeon Vega Gfx"},
    {0x93, "AMD Radeon Vega 1 Graphics"},
    {0xA1, "AMD Radeon Vega 10 Graphics"},
    {0xA2, "AMD Radeon Vega 8 Graphics"},
    {0xA3, "AMD Radeon Vega 6 Graphics"},
    {0xA4, "AMD Radeon Vega 3 Graphics"},
    {0xB1, "AMD Radeon Vega 10 Graphics"},
    {0xB2, "AMD Radeon Vega 8 Graphics"},
    {0xB3, "AMD Radeon Vega 6 Graphics"},
    {0xB4, "AMD Radeon Vega 3 Graphics"},
    {0xC1, "AMD Radeon Vega 10 Graphics"},
    {0xC2, "AMD Radeon Vega 8 Graphics"},
    {0xC3, "AMD Radeon Vega 6 Graphics"},
    {0xC4, "AMD Radeon Vega 3 Graphics"},
    {0xC5, "AMD Radeon Vega 3 Graphics"},
    {0xC8, "AMD Radeon Vega 11 Graphics"},
    {0xC9, "AMD Radeon Vega 8 Graphics"},
    {0xCA, "AMD Radeon Vega 11 Graphics"},
    {0xCB, "AMD Radeon Vega 8 Graphics"},
    {0xCC, "AMD Radeon Vega 3 Graphics"},
    {0xCE, "AMD Radeon Vega 3 Graphics"},
    {0xCF, "AMD Ryzen Embedded R1305G with Radeon Vega Gfx"},
    {0xD1, "AMD Radeon Vega 10 Graphics"},
    {0xD2, "AMD Radeon Vega 8 Graphics"},
    {0xD3, "AMD Radeon Vega 6 Graphics"},
    {0xD4, "AMD Radeon Vega 3 Graphics"},
    {0xD8, "AMD Radeon Vega 11 Graphics"},
    {0xD9, "AMD Radeon Vega 8 Graphics"},
    {0xDA, "AMD Radeon Vega 11 Graphics"},
    {0xDB, "AMD Radeon Vega 3 Graphics"},
    {0xDB, "AMD Radeon Vega 8 Graphics"},
    {0xDC, "AMD Radeon Vega 3 Graphics"},
    {0xDD, "AMD Radeon Vega 3 Graphics"},
    {0xDE, "AMD Radeon Vega 3 Graphics"},
    {0xDF, "AMD Radeon Vega 3 Graphics"},
    {0xE3, "AMD Radeon Vega 3 Graphics"},
    {0xE4, "AMD Ryzen Embedded R1102G with Radeon Vega Gfx"},
};

static constexpr Model dev15dd[] = {
    {0x81, "AMD Ryzen Embedded V1807B with Radeon Vega Gfx"},
    {0x82, "AMD Ryzen Embedded V1756B with Radeon Vega Gfx"},
    {0x83, "AMD Ryzen Embedded V1605B with Radeon Vega Gfx"},
    {0x84, "AMD Radeon Vega 6 Graphics"},
    {0x85, "AMD Ryzen Embedded V1202B with Radeon Vega Gfx"},
    {0x86, "AMD Radeon Vega 11 Graphics"},
    {0x88, "AMD Radeon Vega 8 Graphics"},
    {0xC1, "AMD Radeon Vega 11 Graphics"},
    {0xC2, "AMD Radeon Vega 8 Graphics"},
    {0xC3, "AMD Radeon Vega 3 / 10 Graphics"},
    {0xC4, "AMD Radeon Vega 8 Graphics"},
    {0xC5, "AMD Radeon Vega 3 Graphics"},
    {0xC6, "AMD Radeon Vega 11 Graphics"},
    {0xC8, "AMD Radeon Vega 8 Graphics"},
    {0xC9, "AMD Radeon Vega 11 Graphics"},
    {0xCA, "AMD Radeon Vega 8 Graphics"},
    {0xCB, "AMD Radeon Vega 3 Graphics"},
    {0xCC, "AMD Radeon Vega 6 Graphics"},
    {0xCE, "AMD Radeon Vega 3 Graphics"},
    {0xCF, "AMD Radeon Vega 3 Graphics"},
    {0xD0, "AMD Radeon Vega 10 Graphics"},
    {0xD1, "AMD Radeon Vega 8 Graphics"},
    {0xD3, "AMD Radeon Vega 11 Graphics"},
    {0xD5, "AMD Radeon Vega 8 Graphics"},
    {0xD6, "AMD Radeon Vega 11 Graphics"},
    {0xD7, "AMD Radeon Vega 8 Graphics"},
    {0xD8, "AMD Radeon Vega 3 Graphics"},
    {0xD9, "AMD Radeon Vega 6 Graphics"},
    {0xE1, "AMD Radeon Vega 3 Graphics"},
    {0xE2, "AMD Radeon Vega 3 Graphics"},
};

static constexpr DevicePair devices[] = {
    {0x15d8, dev15d8, arrsize(dev15d8)},
    {0x15dd, dev15dd, arrsize(dev15dd)},
};

inline const char *getBranding(uint16_t dev, uint16_t rev) {
    for (auto &device : devices) {
        if (device.dev == dev) {
            for (size_t i = 0; i < device.modelNum; i++) {
                auto &model = device.models[i];
                if (model.rev == rev) { return model.name; }
            }
            break;
        }
    }

    switch (dev) {
        case 0x15D8:
            return "AMD Radeon RX Vega X (Picasso)";
        case 0x15DD:
            return "AMD Radeon RX Vega X (Raven Ridge)";
        case 0x164C:
            return "AMD Radeon RX Vega X (Lucienne)";
        case 0x1636:
            return "AMD Radeon RX Vega X (Renoir)";
        case 0x15E7:
            return "AMD Radeon RX Vega X (Barcelo)";
        case 0x1638:
            return "AMD Radeon RX Vega X (Cezanne)";
        default:
            return nullptr;
    }
}
