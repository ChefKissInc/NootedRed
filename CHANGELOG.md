# Change log

## v0.8.10 (15/05/2026)

### Enhancements

- Further compatibility work for macOS Mojave.

### Bug Fixes

- Fix conflict of new AppleGFXHDA code with AppleALC's.

> [!NOTE]
> Use the `RELEASE` build. `RESEARCH_RELEASE` is `DEBUG` with optimisations.
> The latter two will be way slower and inefficient than the former build. Use them **only** when debugging an issue with NootedRed.

**Full Changelog**: https://github.com/ChefKissInc/NootedRed/compare/v0.8.9...v0.8.10

## v0.8.9 (15/05/2026)

### Enhancements

- Added the boot argument `-NRedNoAccel` for running FB-only.

  Primarily for debug purposes.
- Miscellaneous improvements to the PenguinWizardry SDK.
- Miscellaneous improvements to the GPUDriversAMD SDK.
- X6000FB and AppleGFXHDA compatibility with macOS Mojave.

  However, the Mojave-compatible X6000FB kext and instructions will only be released on v0.9.0!

### Bug Fixes

- Added missing call to calcAndSetVrrTimestampInfo in GFX9DCNDisplay implementation.

  This missing piece of code is needed on macOS Big Sur and newer, to tell the accelerator about the display timings.
- Fixed the max display count for Renoir to 4 also in the GFX9DCNDisplay implementation.

  I missed it in v0.8.4.

> [!NOTE]
> Use the `RELEASE` build. `RESEARCH_RELEASE` is `DEBUG` with optimisations.
> The latter two will be way slower and inefficient than the former build. Use them **only** when debugging an issue with NootedRed.

**Full Changelog**: https://github.com/ChefKissInc/NootedRed/compare/v0.8.8...v0.8.9

## v0.8.8 (12/05/2026)

### Enhancements

- Moved X6000FB VBIOS fix code out of iVega, into Hotfixes.

### Bug Fixes

- Gated `getTriageHardwareData` behind macOS Big Sur and macOS Monterey.

  It doesn't exist on macOS Catalina. Also replaced usage of `scnprintf` with `snprintf` as it may not be available there and cause an `Invalid Parameter` error when loading the kext.

> [!NOTE]
> Use the `RELEASE` build. `RESEARCH_RELEASE` is `DEBUG` with optimisations.
> The latter two will be way slower and inefficient than the former build. Use them **only** when debugging an issue with NootedRed.

**Full Changelog**: https://github.com/ChefKissInc/NootedRed/compare/v0.8.7...v0.8.8

## v0.8.7 (12/05/2026)

### Bug Fixes

- Neutralised setupBootWatermarks in X6000FB.

  This function applies some hardware quirk through the DCN watermarks on specific RDNA 1 dGPUs. On Raven the register offsets are even completely wrong and changing random DCN registers on specific ASIC revisions. So, I just neutralised the function.
- Added custom implementation for getTriageHardwareData (exists in macOS Monterey and below).

  This function prints a few DCN register values in a GPU diagnosis report, however, the offsets aren't quite matching for Raven and Renoir, along with the max display pipe count being 4.

**NOTE:** The "Research Release" and "Debug" builds will be way slower and inefficient than the "Release" build. Do not use them if not debugging an issue with the software.

**Full Changelog**: https://github.com/ChefKissInc/NootedRed/compare/v0.8.6...v0.8.7

## v0.8.6 (11/05/2026)

### Bug Fixes

- Worked around spurious kernel panic in recovery likely caused by Lilu bug.

**NOTE:** The "Research Release" and "Debug" builds will be way slower and inefficient than the "Release" build. Do not use them if not debugging an issue with the software.

**Full Changelog**: https://github.com/ChefKissInc/NootedRed/compare/v0.8.5...v0.8.6

## v0.8.5 (11/05/2026)

### Enhancements

- Cleaned up X6000FB branding code, removing the subsystem cruft and the weird "Renoir Graphics D4" and similar models.
- Specialised fallback model to "Radeon RX Raven Graphics", "Radeon RX Picasso Graphics", and "Radeon RX Renoir Graphics".

**NOTE:** The "Research Release" and "Debug" builds will be way slower and inefficient than the "Release" build. Do not use them if not debugging an issue with the software.

**Full Changelog**: https://github.com/ChefKissInc/NootedRed/compare/v0.8.4-2...v0.8.5

## v0.8.4 (10/05/2026)

### Enhancements

- New code style for improved readability.
- Some small code cleanup and optimisations.

  This includes improving and fixing up the dm_logger_write reimplementation that's enabled with `-NRedDebug`.
- Replaced useless backtrace on SMU/SMC command errors with plain logs including the command ID.
- Improved the memory type handling inside the X6000FB code.
- Replaced legacy model mechanism with its driver-native counterpart.

### Bug Fixes

- Fixed potential out-of-bounds string comparisons throughout the code.
- Fixed the waitForFunc timeout calculation in nanoseconds being implicitly truncated.
- Fixed the saved framebuffer size calculation of the X5000DCNDisplay implementation being implicitly truncated.
- Fixed copy paste error on the ARGB1555 surface format case of the X5000DCNDisplay implementation.
- Fixed the display pipe count on Renoir-based ASICs.

  Renoir and Raven both have only 4 display pipes. Not sure where I got the previous assumption of 6 for Renoir, maybe accidentally read from DCN 2.0 (Navi) instead of 2.1 (Renoir).

**NOTE:** The "Research Release" and "Debug" builds will be way slower and inefficient than the "Release" build. Do not use them if not debugging an issue with the software.

**Full Changelog**: https://github.com/ChefKissInc/NootedRed/compare/v0.8.3...v0.8.4-2

## v0.8.3 (24/12/2025)

### Enhancements

- Removed redundant `GPUDCCDisplayable` setting.
- Removed unnecessary `PP_ToolsLogSpaceSize` setting. This setting is only applicable for SMU 11.
- Removed redundant `PP_WorkLoadPolicyMask` setting. The value specified was 0, which is already the default.
- Removed `IOPCITunnelCompatible` property.
  iGPUs can't run through Thunderbolt.
- Moved SMU 12 firmware wait to separate function.
- Some TTL event handler cleanup.
- Miscellaneous firmware constant cleanup.

### Bug Fixes

- Rechecked driver settings and adapted them correctly to AMD Adrenaline's.
- Added missing `DalForceSingleDispPipeSplit` to Picasso settings.
- Removed Raven2 RLC A0 firmware. 
- Activate A0 RLC firmware only on non-Picasso or Picasso AM4.
- Disable MMHub PG as per the driver settings.
- Added missing `GPUTaskSingleChannel` accelerator setting.
  Possibly added in Sonoma or something, non-existent in Big Sur, default false, but set to true in the accelerator kexts (Navi and Vega).
- Corrected "wait for" logic.
  Corrected timeout to PP default of 2s from Linux's 100s and used IODelay instead of IOSleep.

**NOTE:** The "Research Release" and "Debug" builds will be way slower and inefficient than the "Release" build. Do not use them if not debugging an issue with the software.

**Full Changelog**: https://github.com/ChefKissInc/NootedRed/compare/v0.8.2...v0.8.3

## v0.8.2 (20/11/2025)

### Enhancements

- Code clean-up.
- Add back the `-NRedOff`, `-NRedBeta` boot arguments.

### Bug Fixes

- Fix cut off strings in `stringifyCRTHWDepth`.

**NOTE:** The "Research Release" and "Debug" builds will be way slower and inefficient than the "Release" build. Do not use them if not debugging an issue with the software.

**Full Changelog**: https://github.com/ChefKissInc/NootedRed/compare/v0.8.1...v0.8.2

## v0.8.1 (17/11/2025)

### Enhancements

- Code clean-up.
- Added debug prints for pixel mode, format, depth for display issue debugging.

### Bug Fixes

- Fixed AUX/OLED backlight code always returning failure.
- Fixed AUX/OLED brightness transition time (15 seconds -> 50 milliseconds).
- Fixed the addrlib setting `depthPipeXorDisable` being set to true on all Raven/Non-Renoir even Raven2 instead of Raven1, which was incorrect.

**NOTE:** The "Research Release" and "Debug" builds will be way slower and inefficient than the "Release" build. Do not use them if not debugging an issue with the software.

**Full Changelog**: https://github.com/ChefKissInc/NootedRed/compare/v0.8.0...v0.8.1

## v0.8.0 (15/11/2025)

After the latest major update, I consider NRed somewhat stable enough, so... here's the very first ever NRed release.

**NOTE:** The "Research Release" and "Debug" builds will be way slower and inefficient than the "Release" build. Do not use them if not debugging an issue with the software.
