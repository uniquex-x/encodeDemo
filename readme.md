## 音频编码算法性能对比研究

### 目标
- （1）支持mac/windows系统下设备可直接编译输出windows/linux/android平台的音频编码动态库
- （2）针对三类音频编码算法进行性能对比，包括FLAC、Vorbis、MP3(LAME)

### 性能指标
**(1) 重点对比以下性能指标**
- **编码效率**: 编码速度、CPU/内存占用
- **编码效果**: 压缩比、文件大小、音质(未来将支持PESQ、POLQA)

**(2) 编码参数影响**
这些算法编码时有哪些参数？这些参数会给这些性能带来哪些影响也进行测试

### 项目目录介绍
**(1) 简单概述**
*相比原项目采用configure编译项目，我们在vorbis,flac均利用**bash脚本+cmake**实现了多平台输出且支持交叉编译能力*
- **flac**: FLAC无损音频编码器源码及编译脚本
  - 没有写交叉编译的脚本，要编译平台端库，直接使用自带的CMakeLists.txt即可.支持macOS平台编译输出mac动态库
  - 交叉编译功能待实现
- **lame**: LAME MP3编码器源码及编译脚本
  - 有交叉编译教程，具体可查看BUILD_README.md. 支持macOS平台编译输出mac、android64位、android32位、windows(待验证)动态库
  - 有windows端脚本
- **vorbis**: Vorbis音频编码器源码及编译脚本
  - 也提供了交叉编译脚本，具体可查看README.md. 支持macOS平台编译输出mac、android64位、android32位、
注：vorbis中提供quick_build.sh提供了交互逻辑，lame没有提供交互逻辑

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
│       ├── encoderManager.*   # 编码器测试框架
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

rm -rf build/  #清除编译文件
```

**(3) 运行性能测试**
```bash
./audio_encoder_test
```

### 新增功能特性

**(1) 交互式测试菜单**
程序现在支持交互式菜单，用户可以选择不同的测试模式：
- 测试所有编码器（默认参数）
- 测试单个编码器（用户指定参数）
- 编码器参数范围测试
- 显示编码器详细信息

**(2) 单个编码器测试**
```cpp
// 示例：测试FLAC编码器，质量参数为8
encoderManager encoderManager("samples/natureStory.wav");
encoderManager.testSingleEncoderWithParams(EncoderType::FLAC, 8);
```

**(3) 参数范围测试**
```cpp
// 示例：测试MP3编码器质量参数0-9，步长为2
encoderManager encoderManager("samples/natureStory.wav");
encoderManager.testEncoderQualityRange(EncoderType::LAME, 0, 9, 2);
```

**(4) 编码上下文支持**
```cpp
// 使用编码上下文进行测试
EncoderParamContext context;
context.encoderType = EncoderType::VORBIS;
context.quality = 6;
encoderManager.testSingleEncoderWithContext(context);
```

### 依赖库版本

- FLAC: 1.5.0
- LAME: 3.99.5  
- libvorbis: 1.3.7
- libogg: 1.3.6