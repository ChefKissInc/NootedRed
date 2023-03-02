# WhateverRed ![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/NootInc/WhateverRed/main.yml?branch=master&logo=github&style=for-the-badge)

An AMD iGPU support [Lilu](https://github.com/acidanthera/Lilu) plugin.

The Source Code of this Original Work is licensed under the `Thou Shalt Not Profit License version 1.0`. See `LICENSE`

## The following parts of the source code have been taken from [WhateverGreen](https://github.com/Acidanthera/WhateverGreen)

- Apple Graphics Device Policy (AGDP) Piker-Alpha (agdpmod=pikera) patch

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

Injecting the GPU kexts is not possible during the OpenCore injection stage. The prelink stage fails for kexts of this type as their dependencies aren't contained in the Boot Kext Collection, where OpenCore injects kexts to, they're in the Auxiliary Kext Collection.

In conclusion, this kext is constricted to Big Sur since there are too many incompatibilities with older and newer macOS versions.

## Project members

- [@ChefKissInc](https://github.com/ChefKissInc) | Project lead, Linux shitcode analyser and kernel extension developer. Extensive knowledge of OS inner workings
- [@NyanCatTW1](https://github.com/NyanCatTW1) | Reverse Engineering and Python automation magician. His Ghidra RedMetaClassAnalyzer script has made the entire process way painless by automagically discovering C++ v-tables for classes.
