# WhateverRed

AMD iGPU [Lilu](https://github.com/acidanthera/Lilu) plug-in.

## !! IMPORTANT

**THIS KEXT CONTAINS REMOTE DEBUGGING VIA CUSTOM NETDBG LOGGING SERVICE. YOU MUST SPECIFY IP AND PORT OR DISABLE AS DESCRIBED BELOW.**

WhateverRed NetDbg is a custom TCP logger developed by project owner [@ChefKissInc](https://github.com/ChefKissInc).

You can find the source code [here](https://github.com/NootInc/WhateverRed-NETDBG)

Specify NetDbg IP and Port via `netdbg_ip_X` and `netdbg_port` boot args.

Where `X` is IP part, i.e IP `1.2.3.4` becomes `netdbg_ip_1=1` `netdbg_ip_2=2` `netdbg_ip_3=3` `netdbg_ip_4=4`.

Pass `-netdbg_disable` to disable NetDbg.

The WhateverRed kext is licensed under BSD-3.

This kext is currently under active research and development. It is not functional, but in a close state to such.

This kext is currently aimed to enable AMD iGPU support for Renoir/Raven(2) and their derivatives (i.e. Picasso).

Unfortunately, due to the complexity of such drivers, extended support for non-existent logic is not possible. Therefore, you are (currently) required to use Catalina to Big Sur (recommended) as it appears Monterey has removed the logic for the iGPUs.

The aforementioned requirement may be bypassed in a later stage by, for example, injecting the kext via the OpenCore Kext injection.

Honourable mention:

[@NyanCatTW1](https://github.com/NyanCatTW1) for helping with the Reverse Engineering and understanding of the AMDGPU logic, and his Ghidra RedMetaClassAnalyzer script, which has made the entire process way painless by automagically discovering C++ v-tables for classes.
