# NootedRed ![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/NootInc/NootedRed/main.yml?branch=master&logo=github&style=for-the-badge)

The AMD Vega iGPU support [Lilu](https://github.com/acidanthera/Lilu) (1.6.4+) plug-in.

The Source Code of this Original Work is licensed under the `Thou Shalt Not Profit License version 1.0`. See [`LICENSE`](https://github.com/NootInc/NootedRed/blob/master/LICENSE)

Thanks [Acidanthera](https://github.com/Acidanthera) for the AppleBacklight code, frame-buffer zero-fill fix, and UnfairGVA patches in [WhateverGreen](https://github.com/Acidanthera/WhateverGreen).

## Prerequisites

- Increase VRAM size; otherwise, the device will fail to initialise. 512MiB VRAM is the minimum, 1GiB or more for proper functionality
- Disable Legacy Boot, also known as CSM; otherwise, you will experience various difficulties, such as a kernel panic with "Failed to get VBIOS from VRAM" as the message
- **Remove** WhateverGreen. **This kext conflicts with it**

## Recommendations

- From this repository, add [`SSDT-PNLF.aml`](Assets/SSDT-PNLF.aml) and [`SSDT-ALS0.aml`](Assets/SSDT-ALS0.aml) if you have no Ambient Light Sensor, along with [`SMCLightSensor.kext`](https://github.com/Acidanthera/VirtualSMC) for backlight functionality. Usually only works on laptops. Add [`BrightnessKeys.kext`](https://github.com/Acidanthera/BrightnessKeys) for brightness control from the keyboard
- Use `MacBookPro16,3` or `MacPro7,1` SMBIOS
- Update to latest macOS 11 (Big Sur) version

## FAQ

### Can I have an AMD dGPU installed on the system?

Your system must not have a GCN 5 or RDNA AMD dGPU, as this kext will conflict with them; we are mixing AMDRadeonX5000 for GCN 5, AMDRadeonX6000 for VCN, and AMDRadeonX6000Framebuffer for DCN. Disable them using `-wegnoegpu` (yes, this works here too) or by adding the `disable-gpu` device property.

### How functional is the kext?

This project is under active research and development; There will be crashes here and there.

The kext is mostly fully functional on all Vega (Raven/Raven2/Picasso/Renoir etc) iGPUs.

See repository issues for more information.

### On which macOS versions am I able to use this on?

Due to the complexity and secrecy of the Metal drivers, adding support for non-existent logic is basically impossible. In addition, the required logic for our iGPUs has been purged from the AMD kexts since Monterey. This cannot be resolved without breaking macOS' integrity and potentially even stability, at the moment.

Injecting the GPU kexts is not possible during the OpenCore injection stage; the pre-linked injection fails for kexts that don't contain their dependencies in the Boot Kext Collection, which is where OpenCore injects kexts to.

In conclusion, this kext works only natively in macOS 11 (Big Sur). For Monterey, a downgrade of `AMDRadeonX6000Framebuffer.kext` and `AMDRadeonX5000HWLibs.kext` is required.
