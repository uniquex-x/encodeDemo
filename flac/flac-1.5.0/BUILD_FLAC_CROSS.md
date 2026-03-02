# FLAC Cross-Build Helper

This script wraps CMake to build FLAC 1.5.0 for common native and cross targets. It mirrors the LAME helper in this repo.

## Script
- Location: build_flac.sh
- Default install path: build/<platform>-<arch>/install under the script directory
- Defaults favor cross builds: examples/tests/docs/manpages are off; programs are on; shared libs are on.

## Prerequisites
- CMake 3.16+ recommended
- Matching cross toolchains on PATH:
  - Linux aarch64: aarch64-linux-gnu-gcc / g++
  - Linux armv7: arm-linux-gnueabihf-gcc / g++
  - Windows: x86_64-w64-mingw32-gcc (or i686-w64-mingw32-gcc)
  - Android: Android NDK r23+ (toolchain file at build/cmake/android.toolchain.cmake)
- Optional: libogg dev libs when WITH_OGG=ON

## Usage
```
./build_flac.sh [options]
```
Key flags:
- --platform native|linux|macos|windows|android
- --arch native|x86|x86_64|armv7|armv8
- --build-type Debug|Release|RelWithDebInfo|MinSizeRel
- --shared ON|OFF (default ON)
- --programs ON|OFF (build flac/metaflac)
- --examples ON|OFF (default OFF)
- --tests ON|OFF (default OFF)
- --docs ON|OFF (default OFF)
- --cxxlibs ON|OFF (libFLAC++)
- --ogg ON|OFF (default ON)
- --no-ogg (shorthand for --ogg OFF)
- --manpages ON|OFF (default OFF when cross-compiling)
- --threads ON|OFF (pthreads)
- --install-prefix <path>
- --ndk-path <path> (Android)
- --api-level <level> (Android, default 21)
- --clean (remove target build dir)

## Examples
- Native macOS (universal defaults):
  - `./build_flac.sh --ogg OFF` or
  - `./build_flac.sh -DOGG_ROOT=/path/to/ogg`
- Linux aarch64 cross (shared off, programs on):
  - `./build_flac.sh --platform linux --arch armv8 --shared OFF  --ogg OFF`
- Android armv8 with NDK:
  - `./build_flac.sh --platform android --arch armv8 --ndk-path /path/to/android-ndk-r26 --build-type Release --ogg ON|OFF`
- Windows x86_64 via MinGW-w64:
  - `./build_flac.sh --platform windows --arch x86_64 --programs ON --shared ON --ogg OFF`

## Notes
- INSTALL_MANPAGES defaults to OFF when CMAKE_CROSSCOMPILING; enable with --manpages ON if pandoc/man sources are available.
- BUILD_DOCS=ON requires Doxygen + Pandoc; keep OFF for most cross builds.
- If you need static libs only, set --shared OFF.
- Adjust --install-prefix to place artifacts where your SDK expects them.
