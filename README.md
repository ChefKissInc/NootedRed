# WhateverRed

AMD iGPU support [Lilu](https://github.com/acidanthera/Lilu) plug-in.

## ⚠️ IMPORTANT ⚠️

**This Kernel Extension contains remote debug logging functionality via our custom "NetDbg" software; you must specify a valid NetDbg server IP or explicitly disable the functionality as described below.**

WhateverRed NetDbg is a custom remote debug logging service, which sends the raw data over TCP.

The first version and idea was developed by [@NyanCatTW1](https://github.com/NyanCatTW1), in Python. However, it was often unstable and incredibly primitive.

The current release is developed by project owner [@ChefKissInc](https://github.com/ChefKissInc), in Rust.

You can find the source code for the service [here](https://github.com/NootInc/WhateverRed-NETDBG).

Specify the NetDbg server IP and port via `netdbg_ip_X` and `netdbg_port` boot args.

Where `X` is IP part. Example: IP `192.168.60.231` would become `netdbg_ip_1=192` `netdbg_ip_2=168` `netdbg_ip_3=60` `netdbg_ip_4=231`.

Pass `-wrednetdbg` to disable the NetDbg functionality.

The WhateverRed kext is licensed under BSD-3.

This project is currently under active research and development. It is not functional, but in a close state to.

Its aim is to enable AMD iGPU support for Renoir/Raven(2) and their derivatives, like Picasso.

Unfortunately, due to the complexity of GPU drivers, especially ones that do not contain public source code distributions, extended support for non-existent logic is not possible. Therefore, you are required to use Catalina (Minimum) to Big Sur (recommended) as it appears the small part of the required iGPU logic has been removed since Monterey.

The OpenCore injection stage is too early for injecting these types of kernel extensions.

Collaborators:

* [@ChefKissInc](https://github.com/ChefKissInc) | Project lead, Linux shitcode analyser and kernel extension developer. Extensive knowledge of OS inner workings
* [@NyanCatTW1](https://github.com/NyanCatTW1) | Reverse Engineering and Python automation magician. His Ghidra RedMetaClassAnalyzer script has made the entire process way painless by automagically discovering C++ v-tables for classes.
