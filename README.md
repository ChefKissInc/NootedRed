# NootedRed ![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/NootInc/NootedRed/main.yml?branch=master&logo=github&style=for-the-badge)

The AMD Vega iGPU support [Lilu](https://github.com/acidanthera/Lilu) (1.6.4+) plug-in.

Supports the entire Vega Raven ASIC family (Ryzen 1xxx (Athlon Silver/Gold) to 5xxx and 7x30 series), from Big Sur to Sonoma.

The Source Code of this Original Work is licensed under the `Thou Shalt Not Profit License version 1.0`. See [`LICENSE`](https://github.com/NootInc/NootedRed/blob/master/LICENSE).

This project is under active research and development; There will be crashes here and there.

See repository issues for more information.

Thanks [Acidanthera](https://github.com/Acidanthera) for the AppleBacklight code and UnfairGVA patches in [WhateverGreen](https://github.com/Acidanthera/WhateverGreen).

## Prerequisites

- Your system must not have a GCN 5 or RDNA AMD dGPU, as this kext will conflict with them; we are mixing AMDRadeonX5000 for GCN 5, AMDRadeonX6000 for VCN, and AMDRadeonX6000Framebuffer for DCN. Disable them using `-wegnoegpu` (yes, this works here too) or by adding the `disable-gpu` device property.
- Increase **VRAM size**; otherwise, the device will fail to initialise. 512MiB VRAM is the minimum, 1GiB or more for proper functionality
- Disable **Legacy Boot**, also known as CSM; otherwise, you will experience various difficulties, such as a kernel panic with "Failed to get VBIOS from VRAM" as the message.
- Remove **`WhateverGreen`**; this kext conflicts with it.
- Use `MacBookPro16,3`, `iMac20,1`, or `iMacPro1,1` SMBIOS. `MacPro7,1` often results in black screen at the moment.
- Do your macOS updates (The minor ones, not necessary to go Big Sur -> Monterey)

## Recommendations

- From this repository, add [`SSDT-PNLF.aml`](Assets/SSDT-PNLF.aml), and [`SSDT-ALS0.aml`](Assets/SSDT-ALS0.aml) if you have no Ambient Light Sensor, along with [`SMCLightSensor.kext`](https://github.com/Acidanthera/VirtualSMC) for backlight functionality. Usually only works on laptops. Add [`BrightnessKeys.kext`](https://github.com/Acidanthera/BrightnessKeys) for brightness control from the keyboard
