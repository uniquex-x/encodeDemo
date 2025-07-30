#!/bin/bash

# LAME 构建系统测试脚本

set -e

echo "========================================"
echo "LAME 构建系统测试"
echo "========================================"

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 检查必要文件
echo "检查构建文件..."
files_to_check=(
    "CMakeLists.txt"
    "build_lame.sh"
    "build_lame.bat"
    "BUILD_README.md"
)

for file in "${files_to_check[@]}"; do
    if [[ -f "$file" ]]; then
        echo "✅ $file"
    else
        echo "❌ $file - 文件不存在"
        exit 1
    fi
done

# 检查CMake
echo ""
echo "检查依赖..."
if command -v cmake >/dev/null 2>&1; then
    cmake_version=$(cmake --version | head -n1)
    echo "✅ CMake: $cmake_version"
else
    echo "❌ CMake 未安装"
    exit 1
fi

# 检查编译器
if command -v gcc >/dev/null 2>&1; then
    gcc_version=$(gcc --version | head -n1)
    echo "✅ GCC: $gcc_version"
elif command -v clang >/dev/null 2>&1; then
    clang_version=$(clang --version | head -n1)
    echo "✅ Clang: $clang_version"
else
    echo "❌ 未找到C编译器 (GCC/Clang)"
    exit 1
fi

# 测试CMake配置
echo ""
echo "测试CMake配置..."
mkdir -p test_build
cd test_build

if cmake .. -DCMAKE_BUILD_TYPE=Release -DWITH_FRONTEND=OFF; then
    echo "✅ CMake配置成功"
else
    echo "❌ CMake配置失败"
    cd ..
    rm -rf test_build
    exit 1
fi

# 清理测试目录
cd ..
rm -rf test_build

echo ""
echo "========================================"
echo "所有测试通过！"
echo ""
echo "可以使用以下命令开始构建:"
echo "  ./build_lame.sh --help    # 查看帮助"
echo "  ./build_lame.sh           # 本地构建"
echo "========================================" 