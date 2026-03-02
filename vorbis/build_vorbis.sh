#!/bin/bash

# =============================================================================
# Ogg Vorbis 音频库交叉编译脚本
# 支持Android arm64-v8a、armeabi-v7a架构以及原生平台构建
# =============================================================================

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
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

# 显示帮助信息
show_help() {
    cat << EOF
Ogg Vorbis 音频库交叉编译脚本

用法:
    $0 [选项]

选项:
    --platform PLATFORM     指定目标平台 (android|macos|linux|windows)
    --android-abi ABI        Android架构 (arm64-v8a|armeabi-v7a|all)
    --android-ndk PATH       Android NDK路径
    --windows-arch ARCH      Windows目标架构 (x86_64|x86) [默认: x86_64]
    --build-type TYPE        构建类型 (Release|Debug) [默认: Release]
    --shared                 构建动态库 [默认]
    --static                 构建静态库
    --clean                  清理所有构建产物
    --help                   显示此帮助信息

示例:
    # Android arm64-v8a架构构建
    $0 --platform android --android-abi arm64-v8a --android-ndk /path/to/ndk

    # Android所有架构构建
    $0 --platform android --android-abi all --android-ndk /path/to/ndk

    # macOS原生构建
    $0 --platform macos

    # Linux原生构建
    $0 --platform linux

    # Windows交叉编译 (需要安装 mingw-w64: brew install mingw-w64)
    $0 --platform windows
    $0 --platform windows --windows-arch x86 --static

    # 清理构建产物
    $0 --clean

环境变量:
    ANDROID_NDK             Android NDK路径 (可替代 --android-ndk 参数)

EOF
}

# 默认参数
PLATFORM=""
ANDROID_ABI="arm64-v8a"
ANDROID_NDK_PATH=""
WINDOWS_ARCH="x86_64"
BUILD_TYPE="Release"
BUILD_SHARED=ON
CLEAN_BUILD=false

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --platform)
            PLATFORM="$2"
            shift 2
            ;;
        --android-abi)
            ANDROID_ABI="$2"
            shift 2
            ;;
        --android-ndk)
            ANDROID_NDK_PATH="$2"
            shift 2
            ;;
        --windows-arch)
            WINDOWS_ARCH="$2"
            shift 2
            ;;
        --build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --shared)
            BUILD_SHARED=ON
            shift
            ;;
        --static)
            BUILD_SHARED=OFF
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --help)
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

# 清理构建产物
if [ "$CLEAN_BUILD" = true ]; then
    print_info "清理所有构建产物..."
    rm -rf build/
    rm -rf build_*/
    rm -rf install_*/
    print_success "清理完成"
    exit 0
fi

# 验证平台参数
if [ -z "$PLATFORM" ]; then
    # 自动检测平台
    case "$OSTYPE" in
        darwin*)  PLATFORM="macos" ;;
        linux*)   PLATFORM="linux" ;;
        msys*|cygwin*) PLATFORM="windows" ;;
        *)
            print_error "无法自动检测平台，请使用 --platform 参数指定"
            exit 1
            ;;
    esac
    print_info "自动检测到平台: $PLATFORM"
fi

# 验证平台值
case "$PLATFORM" in
    android|macos|linux|windows)
        ;;
    *)
        print_error "不支持的平台: $PLATFORM"
        print_error "支持的平台: android, macos, linux, windows"
        exit 1
        ;;
esac

# Android特定验证
if [ "$PLATFORM" = "android" ]; then
    # 检查NDK路径
    if [ -z "$ANDROID_NDK_PATH" ]; then
        if [ -n "$ANDROID_NDK" ]; then
            ANDROID_NDK_PATH="$ANDROID_NDK"
        else
            print_error "Android构建需要指定NDK路径"
            print_error "使用 --android-ndk 参数或设置 ANDROID_NDK 环境变量"
            exit 1
        fi
    fi
    
    # 验证NDK路径
    if [ ! -d "$ANDROID_NDK_PATH" ]; then
        print_error "NDK路径不存在: $ANDROID_NDK_PATH"
        exit 1
    fi
    
    if [ ! -f "$ANDROID_NDK_PATH/build/cmake/android.toolchain.cmake" ]; then
        print_error "无效的NDK路径，找不到android.toolchain.cmake"
        exit 1
    fi
    
    # 验证ABI
    case "$ANDROID_ABI" in
        arm64-v8a|armeabi-v7a|all)
            ;;
        *)
            print_error "不支持的Android ABI: $ANDROID_ABI"
            print_error "支持的ABI: arm64-v8a, armeabi-v7a, all"
            exit 1
            ;;
    esac
fi

# Windows特定验证
if [ "$PLATFORM" = "windows" ]; then
    case "$WINDOWS_ARCH" in
        x86_64|x86)
            ;;
        *)
            print_error "不支持的Windows架构: $WINDOWS_ARCH"
            print_error "支持的架构: x86_64, x86"
            exit 1
            ;;
    esac
fi

# 验证构建类型
case "$BUILD_TYPE" in
    Release|Debug)
        ;;
    *)
        print_error "不支持的构建类型: $BUILD_TYPE"
        print_error "支持的类型: Release, Debug"
        exit 1
        ;;
esac

# 检查依赖工具
check_dependencies() {
    print_info "检查构建依赖..."
    
    if ! command -v cmake &> /dev/null; then
        print_error "CMake未安装或不在PATH中"
        exit 1
    fi
    
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    print_info "发现CMake版本: $CMAKE_VERSION"
    
    # 检查构建工具
    if command -v make &> /dev/null; then
        print_info "发现构建工具: $(which make)"
    elif command -v ninja &> /dev/null; then
        NINJA_PATH=$(which ninja)
        if [[ "$NINJA_PATH" =~ "depot_tools" ]]; then
            print_warning "检测到depot_tools中的ninja，可能不兼容，建议安装系统ninja"
            print_info "可以通过brew install ninja安装系统ninja"
        else
            print_info "发现构建工具: $NINJA_PATH"
        fi
    else
        print_error "需要make或ninja构建工具"
        exit 1
    fi
    
    # 检查编译器
    if [ "$PLATFORM" = "windows" ]; then
        # Windows 交叉编译需要 MinGW-w64
        if [ "$WINDOWS_ARCH" = "x86_64" ]; then
            MINGW_CC="x86_64-w64-mingw32-gcc"
            MINGW_CXX="x86_64-w64-mingw32-g++"
            MINGW_RC="x86_64-w64-mingw32-windres"
        else
            MINGW_CC="i686-w64-mingw32-gcc"
            MINGW_CXX="i686-w64-mingw32-g++"
            MINGW_RC="i686-w64-mingw32-windres"
        fi

        if ! command -v "$MINGW_CC" &> /dev/null; then
            print_error "未找到 MinGW-w64 交叉编译器: $MINGW_CC"
            print_error "请先安装 mingw-w64:"
            print_error "  macOS:  brew install mingw-w64"
            print_error "  Ubuntu: sudo apt install mingw-w64"
            exit 1
        fi
        print_info "发现 MinGW-w64 C编译器:   $(which $MINGW_CC)"
        print_info "发现 MinGW-w64 C++编译器: $(which $MINGW_CXX)"
    else
        if command -v clang &> /dev/null; then
            print_info "发现编译器: $(which clang)"
        elif command -v gcc &> /dev/null; then
            print_info "发现编译器: $(which gcc)"
        else
            print_error "需要C编译器 (clang或gcc)"
            exit 1
        fi
    fi
    
    print_success "依赖检查通过"
}

# 执行构建
build_single_arch() {
    local arch=$1
    local build_dir="build"
    
    if [ "$PLATFORM" = "android" ]; then
        build_dir="build_android_${arch}"
    else
        build_dir="build_${PLATFORM}"
    fi
    
    print_info "===========================================" 
    if [ "$PLATFORM" = "android" ]; then
        print_info "开始构建 Android $arch 架构"
    else
        print_info "开始构建 $PLATFORM 平台"
    fi
    print_info "构建目录: $build_dir"
    print_info "构建类型: $BUILD_TYPE"
    print_info "动态库: $BUILD_SHARED"
    print_info "==========================================="
    
    # 创建构建目录
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    # 准备CMake参数
    CMAKE_ARGS=(
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DBUILD_SHARED_LIBS="$BUILD_SHARED"
    )
    
    # 根据平台设置编译器和工具链
    if [ "$PLATFORM" = "android" ]; then
        # Android: 使用 NDK 工具链文件，不手动指定编译器
        CMAKE_ARGS+=(
            -DBUILD_ANDROID=ON
            -DANDROID_NDK="$ANDROID_NDK_PATH"
            -DANDROID_ABI="$arch"
        )
    elif [ "$PLATFORM" = "windows" ]; then
        # Windows 交叉编译: 使用 MinGW-w64 工具链
        if [ "$WINDOWS_ARCH" = "x86_64" ]; then
            local mingw_cc="x86_64-w64-mingw32-gcc"
            local mingw_cxx="x86_64-w64-mingw32-g++"
            local mingw_rc="x86_64-w64-mingw32-windres"
        else
            local mingw_cc="i686-w64-mingw32-gcc"
            local mingw_cxx="i686-w64-mingw32-g++"
            local mingw_rc="i686-w64-mingw32-windres"
        fi
        CMAKE_ARGS+=(
            -DCMAKE_SYSTEM_NAME=Windows
            -DCMAKE_C_COMPILER="$mingw_cc"
            -DCMAKE_CXX_COMPILER="$mingw_cxx"
            -DCMAKE_RC_COMPILER="$mingw_rc"
        )
    else
        # macOS / Linux: 明确设置本机编译器（避免编译器检测问题）
        if command -v clang &> /dev/null; then
            CMAKE_ARGS+=(-DCMAKE_C_COMPILER=clang)
            CMAKE_ARGS+=(-DCMAKE_CXX_COMPILER=clang++)
        elif command -v gcc &> /dev/null; then
            CMAKE_ARGS+=(-DCMAKE_C_COMPILER=gcc)
            CMAKE_ARGS+=(-DCMAKE_CXX_COMPILER=g++)
        fi
    fi
    
    # 检测构建系统 - 优先使用make避免depot_tools ninja的问题
    if command -v make &> /dev/null; then
        CMAKE_ARGS+=(-G "Unix Makefiles")
        BUILD_CMD="make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
    elif command -v ninja &> /dev/null && [[ ! $(which ninja) =~ "depot_tools" ]]; then
        CMAKE_ARGS+=(-G "Ninja")
        BUILD_CMD="ninja"
    else
        print_warning "未找到合适的构建工具，使用默认make"
        BUILD_CMD="make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
    fi
    
    # 配置
    print_info "配置构建..."
    cmake .. "${CMAKE_ARGS[@]}"
    
    # 构建
    print_info "开始编译..."
    $BUILD_CMD all_libs
    
    cd ..
    
    if [ "$PLATFORM" = "android" ]; then
        print_success "Android $arch 架构构建完成"
    else
        print_success "$PLATFORM 平台构建完成"
    fi
}

# 主函数
main() {
    print_info "开始Ogg Vorbis音频库交叉编译"
    
    # 检查当前目录
    if [ ! -d "libogg-1.3.6" ] || [ ! -d "libvorbis-1.3.7" ]; then
        print_error "请在包含 libogg-1.3.6 和 libvorbis-1.3.7 目录的根目录下运行此脚本"
        exit 1
    fi
    
    # 检查依赖
    check_dependencies
    
    # 执行构建
    if [ "$PLATFORM" = "android" ] && [ "$ANDROID_ABI" = "all" ]; then
        # 构建所有Android架构
        for abi in arm64-v8a armeabi-v7a; do
            build_single_arch "$abi"
        done
        
        # 显示所有构建结果
        print_success "所有Android架构构建完成"
        echo
        print_info "构建产物位置:"
        for abi in arm64-v8a armeabi-v7a; do
            install_dir="install_android_${abi}"
            if [ -d "$install_dir" ]; then
                echo "  $abi: $PWD/$install_dir"
            fi
        done
    else
        # 构建单个架构或平台
        if [ "$PLATFORM" = "android" ]; then
            build_single_arch "$ANDROID_ABI"
            install_dir="install_android_${ANDROID_ABI}"
        else
            build_single_arch "$PLATFORM"
            install_dir="install_${PLATFORM}"
        fi
        
        # 显示构建结果
        actual_install_dir=""
        if [ -d "$install_dir" ]; then
            actual_install_dir="$install_dir"
        elif [ -d "$build_dir/$install_dir" ]; then
            actual_install_dir="$build_dir/$install_dir"
        fi
        
        if [ -n "$actual_install_dir" ]; then
            echo
            print_success "构建完成！"
            print_info "构建产物位置: $PWD/$actual_install_dir"
            echo
            print_info "库文件:"
            find "$actual_install_dir/lib" -name "*.so" -o -name "*.a" -o -name "*.dylib" -o -name "*.dll" 2>/dev/null | sed 's/^/  /'
            echo
            print_info "头文件:"
            find "$actual_install_dir/include" -name "*.h" 2>/dev/null | sed 's/^/  /'
        fi
    fi
    
    echo
    print_success "全部构建任务完成！"
}

# 运行主函数
main "$@" 