#!/bin/bash

# FLAC cross-platform build helper

set -e

# colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

info()    { echo -e "${BLUE}[INFO]${NC} $1"; }
success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
warn()    { echo -e "${YELLOW}[WARNING]${NC} $1"; }
err()     { echo -e "${RED}[ERROR]${NC} $1"; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="${SCRIPT_DIR}"
BUILD_ROOT="${SCRIPT_DIR}/build"

# defaults
target_platform="native"
target_arch="native"
build_type="Release"
shared_libs="ON"
build_programs="ON"
build_examples="OFF"
build_tests="OFF"
build_docs="OFF"
build_cxxlibs="ON"
with_ogg="ON"
install_manpages="OFF"
enable_threads="ON"
install_prefix="${BUILD_ROOT}/install"
install_prefix_user_set=0

# Android
android_ndk=""
android_api="21"
android_stl="c++_shared"

clean_build=0

usage() {
    cat <<EOF
FLAC cross-platform build script

Usage: $0 [options]

Options:
  -p, --platform PLATFORM   Target platform (native|linux|macos|windows|android)
  -a, --arch ARCH           Target arch (native|x86|x86_64|armv7|armv8)
  -t, --build-type TYPE     CMake build type (Debug|Release|RelWithDebInfo|MinSizeRel)
  -s, --shared ON|OFF       Build shared libs (default: ON)
      --programs ON|OFF     Build flac/metaflac CLI tools (default: ON)
      --examples ON|OFF     Build examples (default: OFF)
      --tests ON|OFF        Build tests (default: OFF)
      --docs ON|OFF         Build docs (default: OFF)
      --cxxlibs ON|OFF      Build libFLAC++ (default: ON)
    --ogg ON|OFF          Enable libogg support (default: ON)
    --no-ogg              Convenience flag to disable libogg detection/build (sets --ogg OFF)
      --manpages ON|OFF     Install manpages (default: OFF for cross)
      --threads ON|OFF      Enable pthread multithreading (default: ON)
    -i, --install-prefix PATH Install prefix (default: build/<platform>-<arch>/install)
  -n, --ndk-path PATH       Android NDK path (required for android)
  -l, --api-level LEVEL     Android API level (default: 21)
  -c, --clean               Remove the existing build dir for this target
  -h, --help                Show this help

Examples:
  # Native build
  $0

  # Linux aarch64 cross (needs aarch64-linux-gnu toolchain)
  $0 --platform linux --arch armv8 --shared OFF

  # Android armv8
  $0 --platform android --arch armv8 --ndk-path /path/to/android-ndk

  # Windows x86_64 via MinGW-w64
  $0 --platform windows --arch x86_64
EOF
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -p|--platform) target_platform="$2"; shift 2;;
            -a|--arch) target_arch="$2"; shift 2;;
            -t|--build-type) build_type="$2"; shift 2;;
            -s|--shared) shared_libs="$2"; shift 2;;
            --programs) build_programs="$2"; shift 2;;
            --examples) build_examples="$2"; shift 2;;
            --tests) build_tests="$2"; shift 2;;
            --docs) build_docs="$2"; shift 2;;
            --cxxlibs) build_cxxlibs="$2"; shift 2;;
            --ogg) with_ogg="$2"; shift 2;;
            --no-ogg) with_ogg="OFF"; shift 1;;
            --manpages) install_manpages="$2"; shift 2;;
            --threads) enable_threads="$2"; shift 2;;
            -i|--install-prefix) install_prefix="$2"; install_prefix_user_set=1; shift 2;;
            -n|--ndk-path) android_ndk="$2"; shift 2;;
            -l|--api-level) android_api="$2"; shift 2;;
            -c|--clean) clean_build=1; shift;;
            -h|--help) usage; exit 0;;
            *) err "Unknown option: $1"; usage; exit 1;;
        esac
    done
}

validate_args() {
    case $target_platform in
        native|linux|macos|windows|android) ;;
        *) err "Unsupported platform: $target_platform"; exit 1;;
    esac

    case $target_arch in
        native|x86|x86_64|armv7|armv8) ;;
        *) err "Unsupported arch: $target_arch"; exit 1;;
    esac

    if [[ "$target_platform" == "android" ]]; then
        if [[ -z "$android_ndk" ]]; then
            err "Android build requires --ndk-path"; exit 1
        fi
        if [[ ! -d "$android_ndk" ]]; then
            err "NDK path not found: $android_ndk"; exit 1
        fi
    fi
}

get_android_abi() {
    case $1 in
        armv7) echo "armeabi-v7a" ;;
        armv8) echo "arm64-v8a" ;;
        x86) echo "x86" ;;
        x86_64) echo "x86_64" ;;
    esac
}

setup_host_defaults() {
    case "$(uname -s)" in
        Darwin) host_os="macos";;
        Linux) host_os="linux";;
        CYGWIN*|MINGW*|MSYS*) host_os="windows";;
        *) host_os="unknown";;
    esac

    case "$(uname -m)" in
        x86_64|amd64) host_arch="x86_64";;
        i386|i686) host_arch="x86";;
        aarch64|arm64) host_arch="armv8";;
        arm*) host_arch="armv7";;
        *) host_arch="unknown";;
    esac

    if [[ "$target_platform" == "native" ]]; then
        target_platform="$host_os"
    fi
    if [[ "$target_arch" == "native" ]]; then
        target_arch="$host_arch"
    fi

    info "Host: ${host_os} (${host_arch}), Target: ${target_platform} (${target_arch})"
}

setup_toolchain() {
    cmake_args=""

    case $target_platform in
        linux)
            cmake_args+=" -DCMAKE_SYSTEM_NAME=Linux"
            case $target_arch in
                armv7)
                    cmake_args+=" -DCMAKE_SYSTEM_PROCESSOR=arm"
                    cmake_args+=" -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc"
                    cmake_args+=" -DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++"
                    ;;
                armv8)
                    cmake_args+=" -DCMAKE_SYSTEM_PROCESSOR=aarch64"
                    cmake_args+=" -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc"
                    cmake_args+=" -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++"
                    ;;
            esac
            ;;
        macos)
            cmake_args+=" -DCMAKE_SYSTEM_NAME=Darwin"
            case $target_arch in
                armv8) cmake_args+=" -DCMAKE_OSX_ARCHITECTURES=arm64";;
                x86_64) cmake_args+=" -DCMAKE_OSX_ARCHITECTURES=x86_64";;
            esac
            ;;
        windows)
            cmake_args+=" -DCMAKE_SYSTEM_NAME=Windows"
            case $target_arch in
                x86_64)
                    cmake_args+=" -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc"
                    cmake_args+=" -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++"
                    ;;
                x86)
                    cmake_args+=" -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc"
                    cmake_args+=" -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++"
                    ;;
            esac
            ;;
        android)
            local android_abi="$(get_android_abi "$target_arch")"
            if [[ -z "$android_abi" ]]; then
                err "Unsupported Android arch: $target_arch"; exit 1
            fi
            local toolchain_file="${android_ndk}/build/cmake/android.toolchain.cmake"
            if [[ ! -f "$toolchain_file" ]]; then
                err "Android toolchain file missing: $toolchain_file"; exit 1
            fi
            cmake_args+=" -DCMAKE_SYSTEM_NAME=Android"
            cmake_args+=" -DCMAKE_ANDROID_NDK=${android_ndk}"
            cmake_args+=" -DCMAKE_ANDROID_ARCH_ABI=${android_abi}"
            cmake_args+=" -DCMAKE_ANDROID_API=${android_api}"
            cmake_args+=" -DCMAKE_ANDROID_STL_TYPE=${android_stl}"
            cmake_args+=" -DCMAKE_TOOLCHAIN_FILE=${toolchain_file}"
            info "Android ABI=${android_abi}, API=${android_api}"
            ;;
    esac
}

setup_build_dir() {
    build_dir="${BUILD_ROOT}/${target_platform}-${target_arch}"
    if [[ $install_prefix_user_set -eq 0 ]]; then
        install_prefix="${build_dir}/install"
    fi
    if [[ $clean_build -eq 1 && -d "$build_dir" ]]; then
        info "Cleaning build dir ${build_dir}"
        rm -rf "$build_dir"
    fi
    mkdir -p "$build_dir"
    info "Build dir: ${build_dir}"
}

configure_project() {
    cd "$build_dir"

    local cmd="cmake"
    cmd+=" -DCMAKE_BUILD_TYPE=${build_type}"
    cmd+=" -DBUILD_SHARED_LIBS=${shared_libs}"
    cmd+=" -DBUILD_PROGRAMS=${build_programs}"
    cmd+=" -DBUILD_EXAMPLES=${build_examples}"
    cmd+=" -DBUILD_TESTING=${build_tests}"
    cmd+=" -DBUILD_DOCS=${build_docs}"
    cmd+=" -DBUILD_CXXLIBS=${build_cxxlibs}"
    cmd+=" -DWITH_OGG=${with_ogg}"
    cmd+=" -DINSTALL_MANPAGES=${install_manpages}"
    cmd+=" -DENABLE_MULTITHREADING=${enable_threads}"
    cmd+=" -DCMAKE_INSTALL_PREFIX=${install_prefix}"
    cmd+=" ${cmake_args} ${SOURCE_DIR}"

    info "CMake: ${cmd}"
    eval ${cmd}
}

build_project() {
    cd "$build_dir"
    local jobs
    if command -v nproc >/dev/null 2>&1; then
        jobs=$(nproc)
    elif command -v sysctl >/dev/null 2>&1; then
        jobs=$(sysctl -n hw.ncpu)
    else
        jobs=4
    fi
    cmake --build . --config ${build_type} --parallel ${jobs}
}

install_project() {
    cd "$build_dir"
    cmake --install . --config ${build_type}
}

print_config() {
    info "Config:"
    info "  Platform: ${target_platform}"
    info "  Arch: ${target_arch}"
    info "  Build type: ${build_type}"
    info "  Shared libs: ${shared_libs}"
    info "  Programs: ${build_programs}"
    info "  Examples: ${build_examples}"
    info "  Tests: ${build_tests}"
    info "  Docs: ${build_docs}"
    info "  C++ libs: ${build_cxxlibs}"
    info "  Ogg: ${with_ogg}"
    info "  Manpages: ${install_manpages}"
    info "  Threads: ${enable_threads}"
    info "  Install: ${install_prefix}"
    if [[ "$target_platform" == "android" ]]; then
        info "  NDK: ${android_ndk}"
        info "  API: ${android_api}"
    fi
}

main() {
    info "FLAC cross build script"
    parse_args "$@"
    validate_args
    setup_host_defaults
    print_config
    setup_toolchain
    setup_build_dir
    configure_project
    build_project
    install_project
    success "Done. Build: ${build_dir} | Install: ${install_prefix}"
}

main "$@"
