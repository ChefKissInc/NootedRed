//
//  kern_model.cpp
//  WhateverGreen
//
//  Copyright © 2017 vit9696. All rights reserved.
//  Copyright © 2022 ChefKiss Inc. All rights reserved.
//

#include <Headers/kern_iokit.hpp>

#include "kern_wred.hpp"

struct Model {
    enum Detect : uint16_t {
        DetectDef = 0x0,
        DetectSub = 0x1,
        DetectRev = 0x2,
        DetectAll = DetectSub | DetectRev,
    };
    uint16_t mode{DetectDef};
    uint16_t subven{0};
    uint16_t sub{0};
    uint16_t rev{0};
    const char *name{nullptr};
};

struct DevicePair {
    uint16_t dev;
    const Model *models;
    size_t modelNum;
};

static constexpr Model dev15dd[] = {
    {Model::DetectRev, 0x0000, 0x0000, 0x00C3, "AMD Radeon Vega 3"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00CB, "AMD Radeon Vega 3"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00CE, "AMD Radeon Vega 3"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00D8, "AMD Radeon Vega 3"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00CC, "AMD Radeon Vega 6"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00D9, "AMD Radeon Vega 6"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00C2, "AMD Radeon Vega 8"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00C4, "AMD Radeon Vega 8"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00C8, "AMD Radeon Vega 8"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00CA, "AMD Radeon Vega 8"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00D1, "AMD Radeon Vega 8"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00D5, "AMD Radeon Vega 8"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00D7, "AMD Radeon Vega 8"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00C3, "AMD Radeon Vega 10"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00D0, "AMD Radeon Vega 10"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00C1, "AMD Radeon Vega 11"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00C6, "AMD Radeon Vega 11"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00C9, "AMD Radeon Vega 11"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00D3, "AMD Radeon Vega 11"},
    {Model::DetectRev, 0x0000, 0x0000, 0x00D6, "AMD Radeon Vega 11"},
    {Model::DetectRev, 0x0000, 0x0000, 0x0081,
     "AMD Ryzen Embedded V1807B with Radeon Vega"},
    {Model::DetectRev, 0x0000, 0x0000, 0x0082,
     "AMD Ryzen Embedded V1756B with Radeon Vega"},
    {Model::DetectRev, 0x0000, 0x0000, 0x0083,
     "AMD Ryzen Embedded V1605B with Radeon Vega"},
    {Model::DetectRev, 0x0000, 0x0000, 0x0085,
     "AMD Ryzen Embedded V1202B with Radeon Vega"},
    {Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD FirePro M6100"},
};

static constexpr DevicePair devices[] = {
    {0x15DD, dev15dd, arrsize(dev15dd)},
};

const char *WRed::getRadeonModel(uint16_t dev, uint16_t rev, uint16_t subven,
                                 uint16_t sub) {
    for (auto &device : devices) {
        if (device.dev == dev) {
            for (size_t j = 0; j < device.modelNum; j++) {
                auto &model = device.models[j];

                if (model.mode & Model::DetectSub &&
                    (model.subven != subven || model.sub != sub))
                    continue;

                if (model.mode & Model::DetectRev && (model.rev != rev))
                    continue;

                return model.name;
            }
            break;
        }
    }

    return nullptr;
}
