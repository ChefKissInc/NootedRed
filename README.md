# WhateverRed

An AMD iGPU support plugin for [Lilu](https://github.com/acidanthera/Lilu).

The Source Code of this Original Work is licensed under the `Thou Shalt Not Profit License version 1.0`. See `LICENSE`

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

Acceleration works, but the internal display is blank. We're only able to control the computer from remote desktop.
Technically still stuck at 99.9999999999999% essentially.

### X5000 vs X6000

- X5000: GFX 9, uses VCE + UVD, and DCE 5
- X6000: GFX 10, uses VCN 2, and DCN 1/2

### Collaborators

- [@ChefKissInc](https://github.com/ChefKissInc) | Project lead, Linux shitcode analyser and kernel extension developer. Extensive knowledge of OS inner workings
- [@NyanCatTW1](https://github.com/NyanCatTW1) | Reverse Engineering and Python automation magician. His Ghidra RedMetaClassAnalyzer script has made the entire process way painless by automagically discovering C++ v-tables for classes.
