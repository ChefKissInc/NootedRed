# Change log

## v0.8.5 (11/05/2026)

### Enhancements

- Cleaned up X6000FB branding code, removing the subsystem cruft and the weird "Renoir Graphics D4" and similar models.
- Specialise fallback model to "Radeon RX Raven Graphics", "Radeon RX Picasso Graphics", and "Radeon RX Renoir Graphics".

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
