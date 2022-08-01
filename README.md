# WhateverRed

AMD iGPU [Lilu](https://github.com/acidanthera/Lilu) plug-in.

The WhateverRed kext is licensed under BSD-3.

This kext is currently under active research and development. It is not functional, but in a close state to such.

This kext is currently aimed to enable AMD iGPU support for Renoir/Raven(2) and their derivatives (i.e. Picasso).

Unfortunately, due to the complexity of such drivers, extended support for non-existent logic is not possible. Therefore, you are (currently) required to use Catalina to Big Sur (recommended) as it appears Monterey has removed the logic for the iGPUs.

The aforementioned requirement may be bypassed in a later stage by, for example, injecting the kext via the OpenCore Kext injection.

Honourable mention:

[@NyanCatTW1](https://github.com/NyanCatTW1) for helping with the Reverse Engineering and understanding of the AMDGPU logic, and his Ghidra RedMetaClassAnalyzer script, which has made the entire process way painless by automagically discovering C++ v-tables for classes.

