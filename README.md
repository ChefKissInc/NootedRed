# WhateverRed

An AMD iGPU support plugin for [Lilu](https://github.com/acidanthera/Lilu).

The Source Code of this Original Work is licensed under the `Thou Shalt Not Profit License version 1.0`. See `LICENSE`

## ⚠️ IMPORTANT ⚠️

This kernel extension currently contains remote debug log functionality through our custom "NetDbg" software. You must specify a valid NetDbg server IP or disable the functionality as described below.

WhateverRed NetDbg is a custom remote debug log service, which sends raw data over TCP. The first version, created in Python by [@NyanCatTW1](https://github.com/NyanCatTW1), was primitive and often unstable. The current release, developed in Rust by project lead [@ChefKissInc](https://github.com/ChefKissInc), is more stable, and has a lot more features (Like a GUI).

You can find the source code for the service [here](https://github.com/NootInc/WhateverRed-NETDBG).

To specify the NetDbg server IP and port, use the `wrednetdbgip` boot argument.

Syntax: `wrednetdbgip=<IP>:<PORT>`

Example: `wrednetdbgip=127.0.0.1:8081`

Pass `wrednetdbg=0` to disable the NetDbg functionality.

## Introduction

We're a 3-people team working on getting graphics acceleration working on AMD iGPU-only hackintoshes, specifically Raven/Raven2/Renoir and their derivatives, such as Picasso.

The existing kexts are patched to achieve this. We are mixing AMDRadeonX5000 (GCN 5 part) and AMDRadeonX6000 (VCN part), and replacing AMD10000Controller (DCE) with AMDRadeonX6000Framebuffer (DCN).

This project is under active research and development; It is not yet functional.

Due to the complexity and secrecy of the Metal 2/3 drivers, adding support for non-existent logic is impossible.

The required logic for our iGPUs has been purged from the AMD kexts since Monterey.

This cannot be resolved as injecting kexts like GPU kexts is impossible during the OpenCore injection stage. The prelink stage fails for kexts of this type as the libraries used for them are not contained in the Boot Kext Collection, which is where OpenCore injects kexts to, they're instead contained in the Auxiliary Kext Collection.

You must use Big Sur since there are too many incompatibilities with older macOS kexts.

## FAQ

### Current Progress

Controller, accelerator and framebuffer are all attached and enabled, GFX engine and SDMA0 engine start up and are able to process command buffers.

However, there is some weird quirk happening relating to GPUVM (GPU Virtual Memory), causing Page Faults in both GFX and ME as soon as WindowServer tries to send an acceleration command. This is the current failure point we are investigating and will eventually fix.

### ETA?

When it is done. It may take more than a month. But we are currently stuck on 99% essentially.

### X5000 vs X6000

- X5000: GFX 9, uses VCE + UVD, and DCE 5
- X6000: GFX 10, uses VCN 2, and DCN 1/2

### Current Status

Everything starts out well and the AMD driver even congratulates us with the following message: `[3:0:0]: Controller is enabled, finish initialization`.

However, as soon as WindowServer tries to zero the video memory, VM page faults occur and cause the SDMA0 and GFX engines to halt, because they fail to read the IB (buffer that contains commands for the target MicroEngine). The AMD driver will attempt to reset the GFX/SDMA0 channels every minute or so, but the page faults persist.

This results in a black screen for a few minutes, before the machine eventually restarts due to a watchdogd timeout since WindowServer fails to check in successfully.

The X6000 branch has been abandoned for a while because its logic is mainly designed for GFX10 (aka. GC 10), whereas our devices run GFX9.

### Collaborators

- [@ChefKissInc](https://github.com/ChefKissInc) | Project lead, Linux shitcode analyser and kernel extension developer. Extensive knowledge of OS inner workings
- [@NyanCatTW1](https://github.com/NyanCatTW1) | Reverse Engineering and Python automation magician. His Ghidra RedMetaClassAnalyzer script has made the entire process way painless by automagically discovering C++ v-tables for classes.
