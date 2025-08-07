## 音频编码算法性能对比研究

### 目标
重点针对三类音频编码算法进行性能对比，包括FLAC、Vorbis、MP3(LAME)

### 性能指标
**(1) 重点对比以下性能指标**
- **编码效率**: 编码速度、CPU/内存占用
- **编码效果**: 压缩比、文件大小、音质(未来将支持PESQ、POLQA)

**(2) 编码参数影响**
这些算法编码时有哪些参数？这些参数会给这些性能带来哪些影响也进行测试

### 项目目录介绍
**(1) 简单概述**
- **flac**: FLAC无损音频编码器源码及编译脚本
  - 没有写交叉编译的脚本，要编译平台端库，直接使用自带的CMakeLists.txt即可
- **lame**: LAME MP3编码器源码及编译脚本
  - 有交叉编译教程，具体可查看BUILD_README.md
- **vorbis**: Vorbis音频编码器源码及编译脚本
  - 也提供了交叉编译脚本，具体可查看README.md

**(2) 项目目录树**
```
compileScript/
├── flac/                    # FLAC编码器源码和编译输出
├── lame/                    # LAME MP3编码器源码和编译输出
├── vorbis/                  # Vorbis编码器源码和编译输出
├── src/                     # 测试程序源码
│   ├── include/             # 编译好的头文件
│   │   ├── flac/           # FLAC头文件
│   │   ├── lame/           # LAME头文件
│   │   └── vorbis/         # Vorbis头文件
│   ├── libs/               # 编译好的库文件
│   │   ├── flac/           # FLAC动态库
│   │   ├── lame/           # LAME动态库
│   │   └── vorbis/         # Vorbis动态库
│   └── test/               # 性能测试程序
│       ├── samples/        # 测试音频样本
│       ├── main.cpp        # 主程序入口
│       ├── encoderTest.*   # 编码器测试框架
│       ├── flacEncoder.*   # FLAC编码器实现
│       ├── mp3Encoder.*    # MP3编码器实现
│       ├── vorbisEncoder.* # Vorbis编码器实现
│       └── CMakeLists.txt  # 构建配置
└── readme.md               # 项目说明文档
```

### 项目环境
均以平台端进行测试，不考虑移动端
- **macOS**: 主要测试平台
- **其他平台**: 待支持

### 编译和运行

**(1) 编译各编码器库**
```bash
# 编译FLAC (在flac/flac-1.5.0目录下)
mkdir build && cd build
cmake .. && make

# 编译LAME (在lame/lame-3.99.5目录下)
mkdir build && cd build
../configure --prefix=$(pwd)/install && make && make install

# 编译Vorbis (在vorbis目录下)
./quick_build.sh  # 或使用build_macos.sh
```

**(2) 编译测试程序**
```bash
cd src/test
mkdir build && cd build
cmake .. && make
```

**(3) 运行性能测试**
```bash
./audio_encoder_test
```

### 测试结果示例

程序将输出类似以下的性能对比结果：

```
=== 编码性能对比结果 ===

编码器         编码时间(ms)  压缩比      输出大小(KB)   CPU时间(s)   内存(MB)
--------------------------------------------------------------------------------
FLAC           1250         2.45        4567          1.234        45
MP3 (LAME)     2100         8.32        1345          2.010        38
Vorbis         1850         7.89        1423          1.756        52

=== 详细分析 ===
🏆 最佳压缩比: MP3 (LAME) (8.32:1)
⚡ 最快编码: FLAC (1250 ms)  
💡 最低CPU使用: FLAC (1.234 s)
```

### 主要特性

1. **统一的编码器接口**: 所有编码器实现相同的接口，便于对比测试
2. **详细的性能指标**: 测量编码时间、压缩比、CPU使用率、内存占用等
3. **资源监控**: 使用系统API监控编码过程中的资源使用情况
4. **自动化测试**: 一次运行测试所有编码器并生成对比报告
5. **扩展性设计**: 易于添加新的编码器或测试指标

### 编码器参数配置

- **FLAC**: 压缩级别 0-8 (8为最高压缩)
- **MP3 (LAME)**: VBR质量 0-9 (0为最高质量)  
- **Vorbis**: 质量参数 -1.0到1.0 (1.0为最高质量)

### 未来计划

1. **音质评估**: 集成PESQ、POLQA等客观音质评估算法
2. **参数优化**: 测试不同编码参数对性能和质量的影响
3. **更多编码器**: 支持AAC、Opus等其他音频编码格式
4. **批量测试**: 支持多个音频文件的批量性能测试
5. **图形化报告**: 生成图表和HTML报告

### 依赖库版本

- FLAC: 1.5.0
- LAME: 3.99.5  
- libvorbis: 1.3.7
- libogg: 1.3.6