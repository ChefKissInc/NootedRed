// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once

enum SignalType {
    SIGNAL_TYPE_NONE = 0,
    SIGNAL_TYPE_DVI_SINGLE_LINK = (1U << 0),
    SIGNAL_TYPE_DVI_DUAL_LINK = (1U << 1),
    SIGNAL_TYPE_HDMI_TYPE_A = (1U << 2),
    SIGNAL_TYPE_LVDS = (1U << 3),
    SIGNAL_TYPE_RGB = (1U << 4),
    SIGNAL_TYPE_DISPLAY_PORT = (1U << 5),
    SIGNAL_TYPE_DISPLAY_PORT_MST = (1U << 6),
    SIGNAL_TYPE_EDP = (1U << 7),
    SIGNAL_TYPE_VIRTUAL = (1U << 9),
};
static_assert(sizeof(SignalType) == 0x4);
