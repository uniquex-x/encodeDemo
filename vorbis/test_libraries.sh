#!/bin/bash

# =============================================================================
# Ogg Vorbis 库测试脚本
# 验证构建的库是否能正常链接和运行
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

# 显示帮助信息
show_help() {
    cat << EOF
Ogg Vorbis 库测试脚本

用法:
    $0 [选项] <安装路径>

选项:
    --help              显示此帮助信息

参数:
    安装路径            构建产物的安装路径（如: install_macos）

示例:
    $0 install_macos
    $0 install_android_arm64-v8a

EOF
}

# 解析命令行参数
INSTALL_PATH=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --help)
            show_help
            exit 0
            ;;
        *)
            if [ -z "$INSTALL_PATH" ]; then
                INSTALL_PATH="$1"
            else
                print_error "未知参数: $1"
                show_help
                exit 1
            fi
            shift
            ;;
    esac
done

# 验证参数
if [ -z "$INSTALL_PATH" ]; then
    print_error "请指定安装路径"
    show_help
    exit 1
fi

if [ ! -d "$INSTALL_PATH" ]; then
    print_error "安装路径不存在: $INSTALL_PATH"
    exit 1
fi

# 创建测试程序
create_test_program() {
    cat > test_vorbis.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

int test_ogg() {
    printf("Testing Ogg library...\n");
    
    ogg_stream_state os;
    if (ogg_stream_init(&os, 12345) != 0) {
        printf("ERROR: Failed to initialize ogg stream\n");
        return 1;
    }
    
    ogg_stream_clear(&os);
    printf("✓ Ogg library test passed\n");
    return 0;
}

int test_vorbis() {
    printf("Testing Vorbis library...\n");
    
    vorbis_info vi;
    vorbis_info_init(&vi);
    
    if (vorbis_encode_init_vbr(&vi, 2, 44100, 0.1f) != 0) {
        printf("ERROR: Failed to initialize vorbis encoder\n");
        vorbis_info_clear(&vi);
        return 1;
    }
    
    vorbis_info_clear(&vi);
    printf("✓ Vorbis library test passed\n");
    return 0;
}

int main() {
    printf("=== Ogg Vorbis Library Test ===\n\n");
    
    printf("Library versions:\n");
    printf("  Vorbis version: %s\n\n", vorbis_version_string());
    
    if (test_ogg() != 0) {
        return 1;
    }
    
    if (test_vorbis() != 0) {
        return 1;
    }
    
    printf("\n✅ All tests passed!\n");
    printf("Libraries are working correctly.\n");
    
    return 0;
}
EOF
}

# 检查库文件存在性
check_libraries() {
    print_info "检查库文件..."
    
    local lib_dir="$INSTALL_PATH/lib"
    local missing_libs=()
    
    # 检查必需的库文件
    local required_libs=(
        "libogg"
        "libvorbis" 
        "libvorbisenc"
        "libvorbisfile"
    )
    
    for lib in "${required_libs[@]}"; do
        local found=false
        for ext in so dylib a dll; do
            if [ -f "$lib_dir/$lib.$ext" ]; then
                found=true
                print_info "找到库文件: $lib.$ext"
                break
            fi
        done
        
        if [ "$found" = false ]; then
            missing_libs+=("$lib")
        fi
    done
    
    if [ ${#missing_libs[@]} -gt 0 ]; then
        print_error "缺少以下库文件:"
        for lib in "${missing_libs[@]}"; do
            echo "  - $lib"
        done
        return 1
    fi
    
    print_success "所有必需的库文件都存在"
    return 0
}

# 检查头文件
check_headers() {
    print_info "检查头文件..."
    
    local include_dir="$INSTALL_PATH/include"
    local missing_headers=()
    
    # 检查必需的头文件
    local required_headers=(
        "ogg/ogg.h"
        "vorbis/codec.h"
        "vorbis/vorbisenc.h"
        "vorbis/vorbisfile.h"
    )
    
    for header in "${required_headers[@]}"; do
        if [ ! -f "$include_dir/$header" ]; then
            missing_headers+=("$header")
        else
            print_info "找到头文件: $header"
        fi
    done
    
    if [ ${#missing_headers[@]} -gt 0 ]; then
        print_error "缺少以下头文件:"
        for header in "${missing_headers[@]}"; do
            echo "  - $header"
        done
        return 1
    fi
    
    print_success "所有必需的头文件都存在"
    return 0
}

# 编译并运行测试程序
compile_and_test() {
    print_info "编译测试程序..."
    
    local include_dir="$INSTALL_PATH/include"
    local lib_dir="$INSTALL_PATH/lib"
    
    # 检查编译器
    local compiler=""
    if command -v gcc &> /dev/null; then
        compiler="gcc"
    elif command -v clang &> /dev/null; then
        compiler="clang"
    else
        print_error "找不到编译器 (gcc 或 clang)"
        return 1
    fi
    
    print_info "使用编译器: $compiler"
    
    # 编译参数
    local compile_flags="-I$include_dir"
    local link_flags="-L$lib_dir"
    
    # 检测库类型并设置链接参数
    if [ -f "$lib_dir/libogg.so" ] || [ -f "$lib_dir/libogg.dylib" ]; then
        link_flags="$link_flags -logg -lvorbis -lvorbisenc -lvorbisfile"
        if [[ "$OSTYPE" == "linux"* ]]; then
            link_flags="$link_flags -Wl,-rpath,$lib_dir"
        fi
    elif [ -f "$lib_dir/libogg.a" ]; then
        link_flags="$link_flags -logg -lvorbis -lvorbisenc -lvorbisfile -lm"
    else
        print_error "无法确定库类型"
        return 1
    fi
    
    # 编译测试程序
    if ! $compiler $compile_flags test_vorbis.c $link_flags -o test_vorbis; then
        print_error "编译失败"
        return 1
    fi
    
    print_success "编译成功"
    
    # 运行测试程序
    print_info "运行测试程序..."
    
    # 设置库路径（用于动态库）
    if [[ "$OSTYPE" == "darwin"* ]]; then
        export DYLD_LIBRARY_PATH="$lib_dir:$DYLD_LIBRARY_PATH"
    else
        export LD_LIBRARY_PATH="$lib_dir:$LD_LIBRARY_PATH"
    fi
    
    if ! ./test_vorbis; then
        print_error "测试程序运行失败"
        return 1
    fi
    
    print_success "测试程序运行成功"
    return 0
}

# 清理测试文件
cleanup() {
    rm -f test_vorbis.c test_vorbis
}

# 主函数
main() {
    print_info "开始测试 Ogg Vorbis 库"
    print_info "安装路径: $INSTALL_PATH"
    echo
    
    # 检查库文件
    if ! check_libraries; then
        exit 1
    fi
    
    echo
    
    # 检查头文件
    if ! check_headers; then
        exit 1
    fi
    
    echo
    
    # 创建测试程序
    create_test_program
    
    # 编译并测试
    if ! compile_and_test; then
        cleanup
        exit 1
    fi
    
    # 清理
    cleanup
    
    echo
    print_success "=========================================="
    print_success "  所有测试通过！库构建成功且可正常使用"
    print_success "=========================================="
}

# 运行主函数
main 