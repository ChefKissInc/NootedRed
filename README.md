# WhateverRed

An AMD iGPU support plugin for [Lilu](https://github.com/acidanthera/Lilu).

The Source Code of this Original Work is licensed under the `Thou Shalt Not Profit License version 1.0`. See `LICENSE`

## ⚠️ IMPORTANT ⚠️

This kernel extension currently contains remote debug log functionality through our custom "NetDbg" software. You must specify a valid NetDbg server IP or disable the functionality as described below.

WhateverRed NetDbg is a custom remote debug log service, which sends raw data over TCP. The first version, created in Python by [@NyanCatTW1](https://github.com/NyanCatTW1), was primitive and often unstable. The current release, developed in Rust by project lead [@ChefKissInc](https://github.com/ChefKissInc), is more stable, and has a lot more features (Like a GUI).

You can find the source code for the service **[here](https://chat.openai.com/chat)**.

To specify the NetDbg server IP and port, use the `wrednetdbgip` boot argument.

Syntax: `wrednetdbgip=<IP>:<PORT>`

Example: `wrednetdbgip=127.0.0.1:8081`

Pass `wrednetdbg=0` to disable the NetDbg functionality.

The WhateverRed kext is licensed under BSD-3.

This project is under active research and development and is not yet functional but close to being so. Its goal is to enable AMD iGPU support for Renoir/Raven(2)/Renoir and their derivatives, such as Picasso. However, due to the complexity of GPU drivers, especially those without public source code, extended support for non-existent logic is not possible. As a result, you must use Catalina (minimum) to Big Sur (recommended) since some required iGPU logic has been removed in Monterey.

Injecting kernel extensions is not possible during the OpenCore injection stage. This is because the prelink injection fails for kernel extensions of this type, as the libraries used for them are not contained in the Boot kernel cache, which is where OpenCore injects kexts to. Instead, they're contained in the Auxiliary kernel cache.


## **Introduction**

We are a team of three working on getting graphics acceleration for AMD iGPUs (Raven/Raven2/Renoir and their derivatives) on hackintoshes. We are patching the existing kexts in order to achieve this (mixing AMDRadeonX5000 for GCN 5 and AMDRadeonX6000 for VCN 1/2, and replacing AMDRadeonX6000Framebuffer for DCN instead of AMD10000Controller which is DCE).

## FAQ

## Current Progress

- Controller, accelerator and framebuffer are all attached and enabled.
- GFX engine and SDMA0 engine start up and are able to process command buffers.
- However, the data being fed to the Virtual Memory Page Table (VMPT) is likely incorrect, causing Page Faults in both GFX and ME as soon as WindowServer tries to clear the video memory.
- We are investigating the cause of the Page Faults and will eventually fix it.

## ETA?

Unknown, the project will be completed when it is done.

## macOS Installation on laptops

It may be possible to install macOS on your laptop, but without graphics acceleration and the touchpad may not work.

## **EFI Setup**

We do not provide support for EFI setup, please follow the AMD section of the Dortania guide and use MacBookPro15,4 as SMBIOS.

## **X5000 vs X6000**

- X5000: GFX 9, uses VCE + UVD, same VBIOS logic as our device
- X6000: GFX 10, uses VCN 2, newer VBIOS structure (displays may not be detected properly)

## **Current Status (X5000)**

- Everything starts out well and the AMD driver even acknowledges the progress.
- However, as soon as WindowServer tries to zero the video memory, Page Faults occur and cause the SDMA0 engine to freeze, breaking the GFX engine and the entire acceleration process.
- The AMD driver will attempt to reset the GFX/SDMA0 channels every minute, but the Page Faults still occur even after the reset.
- This results in a black screen for a few minutes, before the machine eventually restarts due to the driver failing to properly reset the channels.

Note: The X6000 branch has been abandoned for now.

###Collaborators:

* [@ChefKissInc](https://github.com/ChefKissInc) | Project lead, Linux shitcode analyser and kernel extension developer. Extensive knowledge of OS inner workings
* [@NyanCatTW1](https://github.com/NyanCatTW1) | Reverse Engineering and Python automation magician. His Ghidra RedMetaClassAnalyzer script has made the entire process way painless by automagically discovering C++ v-tables for classes.
