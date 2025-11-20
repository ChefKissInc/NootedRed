# Change log

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
