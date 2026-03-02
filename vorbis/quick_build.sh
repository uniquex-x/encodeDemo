#!/bin/bash

# =============================================================================
# Ogg Vorbis 快速构建脚本 - 预设常见构建场景
# =============================================================================

set -e

# 颜色定义
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

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

# 显示菜单
show_menu() {
    echo
    echo "==============================================="
    echo "    Ogg Vorbis 音频库快速构建脚本"
    echo "==============================================="
    echo
    echo "请选择构建场景:"
    echo
    echo "1) Android arm64-v8a (64位ARM)"
    echo "2) Android armeabi-v7a (32位ARM)" 
    echo "3) Android 所有架构 (arm64-v8a + armeabi-v7a)"
    echo "4) macOS 原生构建"
    echo "5) Linux 原生构建"
    echo "6) 清理所有构建产物"
    echo "7) 显示构建帮助"
    echo "8) Windows 交叉编译 (MinGW-w64)"
    echo "0) 退出"
    echo
}

# 检查NDK路径
check_ndk() {
    if [ -z "$ANDROID_NDK" ]; then
        echo
        print_warning "未设置 ANDROID_NDK 环境变量"
        echo "请输入您的Android NDK路径:"
        read -r ndk_path
        if [ ! -d "$ndk_path" ]; then
            print_error "NDK路径不存在: $ndk_path"
            exit 1
        fi
        export ANDROID_NDK="$ndk_path"
        print_info "NDK路径设置为: $ANDROID_NDK"
    else
        print_info "使用NDK路径: $ANDROID_NDK"
    fi
}

# 执行构建
execute_build() {
    local cmd="$1"
    print_info "执行命令: $cmd"
    echo
    eval "$cmd"
}

# 主逻辑
main() {
    while true; do
        show_menu
        echo -n "请选择 (0-8): "
        read -r choice
        
        case $choice in
            1)
                print_info "构建 Android arm64-v8a"
                check_ndk
                execute_build "./build_vorbis.sh --platform android --android-abi arm64-v8a --android-ndk \"$ANDROID_NDK\""
                ;;
            2)
                print_info "构建 Android armeabi-v7a"
                check_ndk
                execute_build "./build_vorbis.sh --platform android --android-abi armeabi-v7a --android-ndk \"$ANDROID_NDK\""
                ;;
            3)
                print_info "构建 Android 所有架构"
                check_ndk
                execute_build "./build_vorbis.sh --platform android --android-abi all --android-ndk \"$ANDROID_NDK\""
                ;;
            4)
                print_info "构建 macOS 原生"
                execute_build "./build_vorbis.sh --platform macos"
                ;;
            5)
                print_info "构建 Linux 原生"
                execute_build "./build_vorbis.sh --platform linux"
                ;;
            6)
                print_info "清理构建产物"
                execute_build "./build_vorbis.sh --clean"
                ;;
            7)
                execute_build "./build_vorbis.sh --help"
                ;;
            8)
                print_info "Windows 交叉编译 (MinGW-w64)"
                execute_build "./build_vorbis.sh --platform windows"
                ;;
            0)
                print_success "退出"
                exit 0
                ;;
            *)
                print_error "无效选择，请输入 0-7"
                ;;
        esac
        
        echo
        echo "按回车键继续..."
        read -r
    done
}

# 检查build_vorbis.sh是否存在
if [ ! -f "build_vorbis.sh" ]; then
    print_error "找不到 build_vorbis.sh 脚本"
    print_error "请确保在正确的目录下运行此脚本"
    exit 1
fi

# 确保build_vorbis.sh可执行
chmod +x build_vorbis.sh

# 运行主函数
main 