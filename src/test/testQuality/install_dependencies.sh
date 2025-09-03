#!/bin/bash

# 音频质量评估工具依赖安装脚本

echo "正在安装音频质量评估工具的依赖..."

# 检查Python版本
python_version=$(python3 --version 2>&1 | awk '{print $2}' | cut -d. -f1,2)
echo "检测到Python版本: $python_version"

# 安装基础依赖
echo "安装基础Python依赖..."
pip3 install numpy scipy librosa soundfile

# 尝试安装PESQ库
echo "尝试安装PESQ库..."
pip3 install pesq

# 检查安装结果
echo "检查依赖安装状态..."

python3 -c "
import sys
import importlib

packages = [
    ('numpy', 'NumPy'),
    ('scipy', 'SciPy'), 
    ('librosa', 'Librosa'),
    ('soundfile', 'SoundFile'),
    ('pesq', 'PESQ')
]

all_installed = True
for package, name in packages:
    try:
        importlib.import_module(package)
        print(f'✓ {name} 已安装')
    except ImportError:
        print(f'✗ {name} 未安装')
        all_installed = False

if all_installed:
    print('\n所有依赖已成功安装！')
else:
    print('\n部分依赖安装失败，但核心功能仍可使用')
"

echo ""
echo "依赖安装完成！"
echo ""
echo "使用示例:"
echo "python3 audio_quality_evaluator.py reference.wav encoded.ogg"
echo "python3 audio_quality_evaluator.py -r original.wav -d compressed.ogg -o results.json"
