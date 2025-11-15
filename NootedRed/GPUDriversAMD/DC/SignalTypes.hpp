// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

enum SignalType {
    SIGNAL_TYPE_NONE = 0,
    SIGNAL_TYPE_DVI_SINGLE_LINK = getBit(0),
    SIGNAL_TYPE_DVI_DUAL_LINK = getBit(1),
    SIGNAL_TYPE_HDMI_TYPE_A = getBit(2),
    SIGNAL_TYPE_LVDS = getBit(3),
    SIGNAL_TYPE_RGB = getBit(4),
    SIGNAL_TYPE_DISPLAY_PORT = getBit(5),
    SIGNAL_TYPE_DISPLAY_PORT_MST = getBit(6),
    SIGNAL_TYPE_EDP = getBit(7),
    SIGNAL_TYPE_VIRTUAL = getBit(9),
};
