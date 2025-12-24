# Change log

## v0.8.3 (24/12/2025)

### Enhancements

- Moved SMU 12 firmware wait to separate function.
- Some TTL event handler cleanup.
- Remove redundant GPUDCCDisplayable setting.
- Miscellaneous firmware constant cleanup.

### Bug Fixes

- Revise driver settings. 
    - Rechecked driver settings and adapted them correctly to AMD Adrenaline's.
    - Added missing `DalForceSingleDispPipeSplit` to Picasso settings.
    - Removed unnecessary `PP_ToolsLogSpaceSize` setting. This setting is only applicable for SMU 11.
    - Removed redundant `PP_WorkLoadPolicyMask` setting. The value specified was 0, which is already the default.
- Removed Raven2 RLC A0 firmware. 
- Activate A0 RLC fw only on non-Picasso or Picasso AM4.
- Disable MMHub PG as per the driver settings.
- Added missing GPUTaskSingleChannel accelerator setting.
  Possibly added in Sonoma or something, non-existent in Big Sur, default false, but set to true in the accelerator kexts (Navi and Vega).
- Removed IOPCITunnelCompatible property.
  iGPUs can't run through Thunderbolt.
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
