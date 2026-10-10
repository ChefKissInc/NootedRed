// Global Kext Info Definitions
//
// Copyright (C) 2025-2026  ChefKiss.
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

#include <Headers/kern_patcher.hpp>
#include <Kexts.hpp>

static const char* pathRadeonX6000Framebuffer =
    "/System/Library/Extensions/AMDRadeonX6000Framebuffer.kext/Contents/MacOS/AMDRadeonX6000Framebuffer";
static const char* pathRadeonX5000HWLibs = "/System/Library/Extensions/AMDRadeonX5000HWServices.kext/Contents/PlugIns/"
                                           "AMDRadeonX5000HWLibs.kext/Contents/MacOS/AMDRadeonX5000HWLibs";
static const char* pathRadeonX5000 = "/System/Library/Extensions/AMDRadeonX5000.kext/Contents/MacOS/AMDRadeonX5000";
static const char* pathBacklight   = "/System/Library/Extensions/AppleBacklight.kext/Contents/MacOS/AppleBacklight";
static const char* pathMCCSControl = "/System/Library/Extensions/AppleMCCSControl.kext/Contents/MacOS/AppleMCCSControl";
static const char* pathAGDP        = "/System/Library/Extensions/AppleGraphicsControl.kext/Contents/PlugIns/"
                                     "AppleGraphicsDevicePolicy.kext/Contents/MacOS/AppleGraphicsDevicePolicy";
static const char* pathAppleGFXHDA = "/System/Library/Extensions/AppleGFXHDA.kext/Contents/MacOS/AppleGFXHDA";

KernelPatcher::KextInfo kextRadeonX6000Framebuffer{
    "com.apple.kext.AMDRadeonX6000Framebuffer", &pathRadeonX6000Framebuffer, 1, {true}, {},
    KernelPatcher::KextInfo::Unloaded,
};

KernelPatcher::KextInfo kextRadeonX5000HWLibs{
    "com.apple.kext.AMDRadeonX5000HWLibs", &pathRadeonX5000HWLibs, 1, {true}, {}, KernelPatcher::KextInfo::Unloaded,
};

KernelPatcher::KextInfo kextRadeonX5000{
    "com.apple.kext.AMDRadeonX5000", &pathRadeonX5000, 1, {true}, {}, KernelPatcher::KextInfo::Unloaded,
};

KernelPatcher::KextInfo kextAppleBacklight{
    "com.apple.driver.AppleBacklight", &pathBacklight, 1, {true}, {}, KernelPatcher::KextInfo::Unloaded,
};

KernelPatcher::KextInfo kextAppleMCCSControl{
    "com.apple.driver.AppleMCCSControl", &pathMCCSControl, 1, {true}, {}, KernelPatcher::KextInfo::Unloaded,
};

KernelPatcher::KextInfo kextAGDP{
    "com.apple.driver.AppleGraphicsDevicePolicy", &pathAGDP, 1, {true}, {}, KernelPatcher::KextInfo::Unloaded,
};

KernelPatcher::KextInfo kextAppleGFXHDA{
    "com.apple.driver.AppleGFXHDA", &pathAppleGFXHDA, 1, {true}, {}, KernelPatcher::KextInfo::Unloaded,
};
