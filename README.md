# LegacyRed

AMD Legacy iGPU [Lilu](https://github.com/acidanthera/Lilu) plug-in.

# IMPORTANT:
**As with WhateverRed, LegacyRed contains a remote debugging system via a custom NETDBG service**
Due to the NETDBG changes within WhateverRed, LegacyRed has been updated to account for the changes, as of such, you must specify a NETDBG IP and Port via boot arguments `netdbg_ip_X` and `netdbg_port`, where X is the part of an IP, example, the IP `12.34.56.78` would be as follows, `netdbg_ip_1=12` `netdbg_ip_2=34` `netdbg_ip_3=56` `netdbg_ip_4=78`

To disable NETDBG entirely, use the boot argument `-netdbg_disable`

Like WhateverRed, LegacyRed is under active development and research, It is not functional. 

Due to the lack of logic on macOS Monterey and up, you are required to run Big Sur or Catalina 

~~In the future, this requirement could be bypassed, eg: OpenCore Injection~~ The stage at which OpenCore injects kexts is too early

# Compatibility
Desktop APUs
 - Empty for now to focus on CzL & Co

Mobile APUs
 - Empty for now to focus on CzL & Co
 
Ultra Mobile APUs
 - Beema
 - Carrizo-L
 - Mullins

# Credits

[The NootInc Team](https://github.com/NootInc) for making and actively working on WhateverRed 
