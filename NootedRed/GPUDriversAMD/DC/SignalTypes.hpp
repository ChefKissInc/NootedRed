// AMD DisplayCore Signal Types
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
#include <Headers/kern_util.hpp>

enum SignalType
{
    SIGNAL_TYPE_NONE             = 0,
    SIGNAL_TYPE_DVI_SINGLE_LINK  = getBit(0),
    SIGNAL_TYPE_DVI_DUAL_LINK    = getBit(1),
    SIGNAL_TYPE_HDMI_TYPE_A      = getBit(2),
    SIGNAL_TYPE_LVDS             = getBit(3),
    SIGNAL_TYPE_RGB              = getBit(4),
    SIGNAL_TYPE_DISPLAY_PORT     = getBit(5),
    SIGNAL_TYPE_DISPLAY_PORT_MST = getBit(6),
    SIGNAL_TYPE_EDP              = getBit(7),
    SIGNAL_TYPE_VIRTUAL          = getBit(9),
};
