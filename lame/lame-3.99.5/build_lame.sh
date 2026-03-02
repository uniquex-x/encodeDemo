#!/bin/bash

# LAME 跨平台构建脚本
# 参考: https://blog.csdn.net/T_T233333333/article/details/147515123
# 支持: Linux, macOS, Windows, Android (armv7, armv8)

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印函数
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="${SCRIPT_DIR}"
BUILD_ROOT="${SCRIPT_DIR}/build"

# 默认配置
TARGET_PLATFORM="native"
TARGET_ARCH="native"
BUILD_TYPE="Release"
SHARED_LIBS="ON"
WITH_FRONTEND="ON"
WITH_DECODER="ON"
INSTALL_PREFIX="${BUILD_ROOT}/install"
INSTALL_PREFIX_USER_SET=0

# Android NDK配置
ANDROID_NDK_PATH=""
ANDROID_API_LEVEL="21"
ANDROID_STL="c++_shared"

# Android ABI映射函数
get_android_abi() {
    case $1 in
        armv7) echo "armeabi-v7a" ;;
        armv8) echo "arm64-v8a" ;;
        x86) echo "x86" ;;
        x86_64) echo "x86_64" ;;
        *) echo "" ;;
    esac
}

get_android_toolchain() {
    case $1 in
        armv7) echo "arm-linux-androideabi" ;;
        armv8) echo "aarch64-linux-android" ;;
        x86) echo "i686-linux-android" ;;
        x86_64) echo "x86_64-linux-android" ;;
        *) echo "" ;;
    esac
}

# 显示帮助信息
show_help() {
    cat << EOF
LAME 跨平台构建脚本

用法: $0 [选项]

选项:
  -p, --platform PLATFORM    目标平台 (native|linux|macos|windows|android)
  -a, --arch ARCH            目标架构 (native|x86|x86_64|armv7|armv8)
  -t, --build-type TYPE       构建类型 (Debug|Release|MinSizeRel|RelWithDebInfo)
  -s, --shared               构建共享库 (ON|OFF, 默认: ON)
  -f, --frontend             构建前端工具 (ON|OFF, 默认: ON)
  -d, --decoder              包含MP3解码器 (ON|OFF, 默认: ON)
    -i, --install-prefix PATH  安装路径 (默认: build/<platform>-<arch>/install)
  -n, --ndk-path PATH        Android NDK路径 (Android构建需要)
  -l, --api-level LEVEL      Android API级别 (默认: 21)
  -c, --clean                清理构建目录
  -h, --help                 显示此帮助信息

示例:
  # 本地构建
  $0

  # Linux x86_64构建
  $0 --platform linux --arch x86_64

  # Android armv8构建
  $0 --platform android --arch armv8 --ndk-path /path/to/ndk

  # Windows交叉编译
  $0 --platform windows --arch x86_64

  # 构建静态库
  $0 --shared OFF

  # 仅构建库，不构建前端
  $0 --frontend OFF
EOF
}

# 解析命令行参数（带值校验，避免漏填导致错位）
require_value() {
    local opt="$1"; shift
    local val="$1"
    if [[ -z "$val" || "$val" == -* ]]; then
        print_error "选项 ${opt} 需要一个值"
        show_help
        exit 1
    fi
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -p|--platform)
                require_value "$1" "$2"
                TARGET_PLATFORM="$2"
                shift 2
                ;;
            -a|--arch)
                require_value "$1" "$2"
                TARGET_ARCH="$2"
                shift 2
                ;;
            -t|--build-type)
                require_value "$1" "$2"
                BUILD_TYPE="$2"
                shift 2
                ;;
            -s|--shared)
                require_value "$1" "$2"
                SHARED_LIBS="$2"
                shift 2
                ;;
            -f|--frontend)
                require_value "$1" "$2"
                WITH_FRONTEND="$2"
                shift 2
                ;;
            -d|--decoder)
                require_value "$1" "$2"
                WITH_DECODER="$2"
                shift 2
                ;;
            -i|--install-prefix)
                require_value "$1" "$2"
                INSTALL_PREFIX="$2"
                INSTALL_PREFIX_USER_SET=1
                shift 2
                ;;
            -n|--ndk-path)
                require_value "$1" "$2"
                ANDROID_NDK_PATH="$2"
                shift 2
                ;;
            -l|--api-level)
                require_value "$1" "$2"
                ANDROID_API_LEVEL="$2"
                shift 2
                ;;
            -c|--clean)
                CLEAN_BUILD=1
                shift
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                print_error "未知参数: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# 验证参数
validate_args() {
    # 验证平台
    case $TARGET_PLATFORM in
        native|linux|macos|windows|android)
            ;;
        *)
            print_error "不支持的平台: $TARGET_PLATFORM"
            exit 1
            ;;
    esac

    # 验证架构
    case $TARGET_ARCH in
        native|x86|x86_64|armv7|armv8)
            ;;
        *)
            print_error "不支持的架构: $TARGET_ARCH"
            exit 1
            ;;
    esac

    # Android构建需要NDK
    if [[ "$TARGET_PLATFORM" == "android" ]]; then
        if [[ -z "$ANDROID_NDK_PATH" ]]; then
            print_error "Android构建需要指定NDK路径"
            exit 1
        fi
        if [[ ! -d "$ANDROID_NDK_PATH" ]]; then
            print_error "NDK路径不存在: $ANDROID_NDK_PATH"
            exit 1
        fi
    fi
}

# 检测系统环境
detect_system() {
    print_info "检测系统环境..."
    
    # 操作系统
    case "$(uname -s)" in
        Darwin)
            HOST_OS="macos"
            ;;
        Linux)
            HOST_OS="linux"
            ;;
        CYGWIN*|MINGW*|MSYS*)
            HOST_OS="windows"
            ;;
        *)
            HOST_OS="unknown"
            ;;
    esac

    # 架构
    case "$(uname -m)" in
        x86_64|amd64)
            HOST_ARCH="x86_64"
            ;;
        i386|i686)
            HOST_ARCH="x86"
            ;;
        aarch64|arm64)
            HOST_ARCH="armv8"
            ;;
        arm*)
            HOST_ARCH="armv7"
            ;;
        *)
            HOST_ARCH="unknown"
            ;;
    esac

    print_info "主机系统: $HOST_OS ($HOST_ARCH)"

    # 如果是native构建，使用主机配置
    if [[ "$TARGET_PLATFORM" == "native" ]]; then
        TARGET_PLATFORM="$HOST_OS"
    fi
    if [[ "$TARGET_ARCH" == "native" ]]; then
        TARGET_ARCH="$HOST_ARCH"
    fi
}

# 设置工具链
setup_toolchain() {
    print_info "设置工具链..."
    
    CMAKE_ARGS=""
    
    case $TARGET_PLATFORM in
        linux)
            setup_linux_toolchain
            ;;
        macos)
            setup_macos_toolchain
            ;;
        windows)
            setup_windows_toolchain
            ;;
        android)
            setup_android_toolchain
            ;;
        *)
            print_info "使用系统默认工具链"
            ;;
    esac
}

# Linux工具链设置
setup_linux_toolchain() {
    print_info "设置Linux工具链 ($TARGET_ARCH)"
    
    case $TARGET_ARCH in
        armv7)
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_SYSTEM_NAME=Linux"
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_SYSTEM_PROCESSOR=arm"
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc"
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++"
            ;;
        armv8)
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_SYSTEM_NAME=Linux"
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_SYSTEM_PROCESSOR=aarch64"
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc"
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++"
            ;;
    esac
}

# macOS工具链设置
setup_macos_toolchain() {
    print_info "设置macOS工具链 ($TARGET_ARCH)"
    
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_SYSTEM_NAME=Darwin"
    
    case $TARGET_ARCH in
        armv8)
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_OSX_ARCHITECTURES=arm64"
            ;;
        x86_64)
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_OSX_ARCHITECTURES=x86_64"
            ;;
    esac
}

# Windows工具链设置
setup_windows_toolchain() {
    print_info "设置Windows工具链 ($TARGET_ARCH)"
    
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_SYSTEM_NAME=Windows"
    
    # 使用MinGW-w64交叉编译工具链
    case $TARGET_ARCH in
        x86_64)
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc"
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++"
            ;;
        x86)
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc"
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++"
            ;;
    esac
}

# Android工具链设置
setup_android_toolchain() {
    print_info "设置Android工具链 ($TARGET_ARCH)"
    
    local android_abi=$(get_android_abi "$TARGET_ARCH")
    if [[ -z "$android_abi" ]]; then
        print_error "不支持的Android架构: $TARGET_ARCH"
        exit 1
    fi
    
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_SYSTEM_NAME=Android"
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_ANDROID_NDK=$ANDROID_NDK_PATH"
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_ANDROID_ARCH_ABI=$android_abi"
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_ANDROID_API=$ANDROID_API_LEVEL"
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_ANDROID_STL_TYPE=$ANDROID_STL"
    
    # 使用NDK工具链文件
    local toolchain_file="$ANDROID_NDK_PATH/build/cmake/android.toolchain.cmake"
    if [[ -f "$toolchain_file" ]]; then
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=$toolchain_file"
    else
        print_error "找不到Android工具链文件: $toolchain_file"
        exit 1
    fi
    
    print_info "Android配置:"
    print_info "  NDK: $ANDROID_NDK_PATH"
    print_info "  ABI: $android_abi"
    print_info "  API Level: $ANDROID_API_LEVEL"
}

# 创建构建目录
setup_build_dir() {
    local build_suffix="${TARGET_PLATFORM}-${TARGET_ARCH}"
    BUILD_DIR="${BUILD_ROOT}/${build_suffix}"

    if [[ $INSTALL_PREFIX_USER_SET -eq 0 ]]; then
        INSTALL_PREFIX="${BUILD_DIR}/install"
    fi
    
    if [[ -n "$CLEAN_BUILD" ]] && [[ -d "$BUILD_DIR" ]]; then
        print_info "清理构建目录: $BUILD_DIR"
        rm -rf "$BUILD_DIR"
    fi
    
    mkdir -p "$BUILD_DIR"
    print_info "构建目录: $BUILD_DIR"
}

# 配置项目
configure_project() {
    print_info "配置项目..."
    
    cd "$BUILD_DIR"
    
    # 基本CMake参数
    local cmake_cmd="cmake"
    cmake_cmd="$cmake_cmd -DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    cmake_cmd="$cmake_cmd -DBUILD_SHARED_LIBS=$SHARED_LIBS"
    cmake_cmd="$cmake_cmd -DWITH_FRONTEND=$WITH_FRONTEND"
    cmake_cmd="$cmake_cmd -DWITH_DECODER=$WITH_DECODER"
    cmake_cmd="$cmake_cmd -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
    
    # 添加工具链参数
    cmake_cmd="$cmake_cmd $CMAKE_ARGS"
    
    # 源码目录
    cmake_cmd="$cmake_cmd $SOURCE_DIR"
    
    print_info "CMake命令: $cmake_cmd"
    eval $cmake_cmd
    
    if [[ $? -eq 0 ]]; then
        print_success "配置完成"
    else
        print_error "配置失败"
        exit 1
    fi
}

# 构建项目
build_project() {
    print_info "构建项目..."
    
    cd "$BUILD_DIR"
    
    # 获取CPU核心数
    if command -v nproc >/dev/null 2>&1; then
        JOBS=$(nproc)
    elif command -v sysctl >/dev/null 2>&1; then
        JOBS=$(sysctl -n hw.ncpu)
    else
        JOBS=4
    fi
    
    cmake --build . --config $BUILD_TYPE --parallel $JOBS
    
    if [[ $? -eq 0 ]]; then
        print_success "构建完成"
    else
        print_error "构建失败"
        exit 1
    fi
}

# 安装项目
install_project() {
    print_info "安装项目..."
    
    cd "$BUILD_DIR"
    cmake --install . --config $BUILD_TYPE
    
    if [[ $? -eq 0 ]]; then
        print_success "安装完成: $INSTALL_PREFIX"
    else
        print_error "安装失败"
        exit 1
    fi
}

# 显示构建信息
show_build_info() {
    print_info "构建配置信息:"
    print_info "  目标平台: $TARGET_PLATFORM"
    print_info "  目标架构: $TARGET_ARCH"
    print_info "  构建类型: $BUILD_TYPE"
    print_info "  共享库: $SHARED_LIBS"
    print_info "  前端工具: $WITH_FRONTEND"
    print_info "  MP3解码器: $WITH_DECODER"
    print_info "  安装路径: $INSTALL_PREFIX"
    
    if [[ "$TARGET_PLATFORM" == "android" ]]; then
        print_info "  Android NDK: $ANDROID_NDK_PATH"
        print_info "  Android API: $ANDROID_API_LEVEL"
    fi
    
    echo
}

# 主函数
main() {
    print_info "LAME 跨平台构建脚本启动"
    
    # 解析参数
    parse_args "$@"
    
    # 验证参数
    validate_args
    
    # 检测系统
    detect_system
    
    # 显示构建信息
    show_build_info
    
    # 设置工具链
    setup_toolchain
    
    # 创建构建目录
    setup_build_dir
    
    # 配置项目
    configure_project
    
    # 构建项目
    build_project
    
    # 安装项目
    install_project
    
    print_success "LAME构建完成！"
    print_info "构建输出位于: $BUILD_DIR"
    print_info "安装文件位于: $INSTALL_PREFIX"
}

# 运行主函数
main "$@" 