# NootedRed ![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/NootInc/NootedRed/main.yml?branch=master&logo=github&style=for-the-badge)

Through hard work come great results.

An AMD iGPU support [Lilu](https://github.com/acidanthera/Lilu) (1.6.4+) plugin.

The Source Code of this Original Work is licensed under the `Thou Shalt Not Profit License version 1.0`. See [`LICENSE`](https://github.com/NootInc/NootedRed/blob/master/LICENSE)

Thanks [Acidanthera](https://github.com/Acidanthera) for the AppleBacklight code, framebuffer init zero-fill fix, and UnfairGVA patches in [WhateverGreen](https://github.com/Acidanthera/WhateverGreen).

**External websites NOT endorsed by us which mention and/or link to Noot and its projects include, but are not limited to,**

- tonymacx86.com,
- technopat.net (technopat.net/sosyal),
- and olarila.com.

Reasons include, but are not limited to, large suspicion due to unrecommended methodology, techniques and methods of distribution, inappropriate behaviour, stealing work, lack of credits and lazy handling of hackintosh issues. We would recommend not supporting websites like those.

## Project members

- [@ChefKissInc](https://github.com/ChefKissInc) | Project lead, Linux shitcode analyser and kernel extension developer. Extensive knowledge of OS inner workings
- [@NyanCatTW1](https://github.com/NyanCatTW1) | Reverse Engineering and Python automation magician. His Ghidra RedMetaClassAnalyzer script has made the entire process way painless by automagically discovering C++ v-tables for classes.

## Recommendations

- Add [SSDT-PNLF.aml](Assets/SSDT-PNLF.aml) by [@ChefKissInc](https://github.com/ChefKissInc) and [@ExtremeXT](https://github.com/ExtremeXT) and compile and add [SSDT-ALS0.aml](https://github.com/Acidanthera/OpenCorePkg/blob/master/Docs/AcpiSamples/Source/SSDT-ALS0.dsl) (NOTE: only if you have no ambient light sensor) along with [SMCLightSensor.kext](https://github.com/Acidanthera/VirtualSMC) by [Acidanthera](https://github.com/Acidanthera) for backlight functionality (usually laptop-only).
- Use `MacBookPro16,3`, `MacBookPro16,4` or `MacPro7,1` SMBIOS.
- Add [AGPMInjector.kext](Assets/AGPMInjector.kext.zip) by [Visual](https://github.com/ChefKissInc). Supports only `MacBookPro16,3`, `MacBookPro16,4` or `MacPro7,1` SMBIOS

## FAQ

### Can I have an AMD dGPU installed on the system?

We are mixing AMDRadeonX5000 for GCN 5, AMDRadeonX6000 for VCN, and AMDRadeonX6000Framebuffer for DCN, so your system must not have a GCN 5 or RDNA AMD dGPU, as this kext will conflict with them.

### How functional is the kext?

This project is under active research and development; There will be crashes here and there, and it is incompatible with Renoir-based iGPUs (Like Cezanne, Lucienne, etc).

The kext is fully functional more or less on Raven/Raven2-based iGPUs (Like Picasso).

See repository issues for more information.

### On which macOS versions am I able to use this on?

Due to the complexity and secrecy of the Metal 2/3 drivers, adding support for non-existent logic is basically impossible.

The required logic for our iGPUs has been purged from the AMD kexts since Monterey.

This cannot be resolved without breaking macOS' integrity and potentially even stability.

Injecting the GPU kexts is not possible during the OpenCore injection stage. The prelink stage fails for kexts of this type as their dependencies aren't contained in the Boot Kext Collection, where OpenCore injects kexts to, they're in the System Kext Collection.

In conclusion, this kext is constricted to Big Sur since there are too many incompatibilities with older and newer macOS versions.

### I get a panic saying "Failed to get VBIOS from VRAM", how can I fix this?

Ensure Legacy Boot/CSM is disabled in your BIOS settings.

### I get stuck on gIOScreenLockState 3, how can I fix this?

Ensure you have VRAM size larger or equal to 512MiB. Recommended VRAM size is 1GiB or larger. (Use a tool like Smokeless-UMAF if there's no option, or options are lacking).
