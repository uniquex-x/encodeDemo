# LAME 现代化构建系统

本文档介绍LAME MP3编码器的现代化构建系统，支持CMake和跨平台交叉编译。

## 概述

我们为LAME项目创建了现代化的构建系统，相较于原有的autotools系统，提供了以下优势：

- ✅ **CMake支持**: 现代化的构建配置
- ✅ **跨平台支持**: Linux、macOS、Windows、Android
- ✅ **多架构支持**: x86、x86_64、ARMv7、ARMv8
- ✅ **交叉编译**: 参考[FFmpeg交叉编译](https://blog.csdn.net/T_T233333333/article/details/147515123)的最佳实践
- ✅ **自动化脚本**: 一键构建和配置

## 文件说明

### 核心文件
- `CMakeLists.txt` - 现代化的CMake构建配置
- `build_lame.sh` - Unix/Linux/macOS跨平台构建脚本
- `build_lame.bat` - Windows构建脚本

### 原有构建系统
- `configure.in` + `Makefile.am` - 传统autotools系统（仍可使用）
- `Makefile.unix` - Unix平台Makefile
- `Makefile.MSVC` - Windows MSVC Makefile

## 快速开始

### 1. 本地构建（推荐）

#### Linux/macOS
```bash
# 使用默认配置（本地架构）
./build_lame.sh

# 查看帮助
./build_lame.sh --help
```

#### Windows
```cmd
REM 使用默认配置
build_lame.bat

REM 查看帮助
build_lame.bat --help
```

### 2. 使用CMake直接构建

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build . --parallel

# 安装
cmake --install . --prefix /usr/local
```

## 支持的平台和架构

| 平台 | 架构 | 状态 | 说明 |
|------|------|------|------|
| Linux | x86_64 | ✅ | 本地编译 |
| Linux | ARMv7 | ✅ | 交叉编译 |
| Linux | ARMv8 | ✅ | 交叉编译 |
| macOS | x86_64 | ✅ | Intel Mac |
| macOS | ARM64 | ✅ | Apple Silicon |
| Windows | x86 | ✅ | 32位Windows |
| Windows | x86_64 | ✅ | 64位Windows |
| Android | ARMv7 | ✅ | armeabi-v7a |
| Android | ARMv8 | ✅ | arm64-v8a |
| Android | x86 | ✅ | x86模拟器 |
| Android | x86_64 | ✅ | x86_64模拟器 |

## 构建选项

### CMake选项
```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \          # Debug|Release|MinSizeRel|RelWithDebInfo
    -DBUILD_SHARED_LIBS=ON \              # 构建共享库(ON)或静态库(OFF)
    -DWITH_DECODER=ON \                   # 包含MP3解码器
    -DWITH_FRONTEND=ON \                  # 构建命令行工具
    -DWITH_MP3X=OFF \                     # 构建MP3分析工具
    -DWITH_MP3RTP=OFF                     # 构建RTP流媒体工具
```

### 脚本选项
```bash
./build_lame.sh \
    --platform linux \                   # native|linux|macos|windows|android
    --arch x86_64 \                      # native|x86|x86_64|armv7|armv8
    --build-type Release \               # Debug|Release|MinSizeRel|RelWithDebInfo
    --shared ON \                        # 共享库(ON)|静态库(OFF)
    --frontend ON \                      # 构建前端工具
    --decoder ON \                       # 包含解码器
    --install-prefix /usr/local \        # 安装路径
    --clean                              # 清理构建目录
```

## 交叉编译示例

### Android构建
```bash
# 设置NDK路径
export ANDROID_NDK=/path/to/android-ndk

# ARMv8 (arm64-v8a)
./build_lame.sh \
    --platform android \
    --arch armv8 \
    --ndk-path $ANDROID_NDK \
    --api-level 21

# ARMv7 (armeabi-v7a) 
./build_lame.sh \
    --platform android \
    --arch armv7 \
    --ndk-path $ANDROID_NDK \
    --api-level 21
```

### Linux ARM交叉编译
```bash
# 安装交叉编译工具链
sudo apt install gcc-arm-linux-gnueabihf gcc-aarch64-linux-gnu

# ARMv7交叉编译
./build_lame.sh --platform linux --arch armv7

# ARMv8交叉编译
./build_lame.sh --platform linux --arch armv8
```

### Windows交叉编译
```bash
# 安装MinGW-w64
sudo apt install mingw-w64

# Windows x86_64
./build_lame.sh --platform windows --arch x86_64

# Windows x86
./build_lame.sh --platform windows --arch x86
```

### macOS通用二进制
```bash
# Intel Mac
./build_lame.sh --platform macos --arch x86_64

# Apple Silicon
./build_lame.sh --platform macos --arch armv8
```

## 依赖要求

### 基本依赖
- CMake 3.16+
- C编译器（GCC、Clang、MSVC）
- Make或Ninja构建工具

### 交叉编译依赖

#### Android
- Android NDK r21+
- CMake 3.21+ (更好的Android支持)

#### Linux ARM
```bash
# Ubuntu/Debian
sudo apt install gcc-arm-linux-gnueabihf gcc-aarch64-linux-gnu

# CentOS/RHEL
sudo yum install gcc-arm-linux-gnu gcc-aarch64-linux-gnu
```

#### Windows (Linux主机)
```bash
# Ubuntu/Debian
sudo apt install mingw-w64

# CentOS/RHEL
sudo yum install mingw64-gcc
```

## 输出文件

构建完成后，输出文件位于：

```
build/
├── android-armv8/          # Android ARM64构建
├── linux-x86_64/          # Linux x86_64构建  
├── windows-x86_64/        # Windows x86_64构建
└── install/               # 安装文件
    ├── bin/
    │   └── lame           # 命令行工具
    ├── lib/
    │   ├── libmp3lame.so  # 共享库（Linux）
    │   ├── libmp3lame.dylib # 共享库（macOS）
    │   ├── libmp3lame.dll # 共享库（Windows）
    │   └── libmp3lame.a   # 静态库（如果启用）
    └── include/
        └── lame.h         # 头文件
```

## 库的使用方法

### 1. 命令行工具使用

构建完成后可以直接使用LAME命令行工具：

```bash
# 设置库路径（macOS/Linux）
export DYLD_LIBRARY_PATH=build/install/lib:$DYLD_LIBRARY_PATH  # macOS
export LD_LIBRARY_PATH=build/install/lib:$LD_LIBRARY_PATH      # Linux

# 使用LAME编码
./build/install/bin/lame input.wav output.mp3

# 查看帮助
./build/install/bin/lame --help

# 高质量VBR编码
./build/install/bin/lame -V2 input.wav output.mp3

# 固定比特率编码
./build/install/bin/lame -b 320 input.wav output.mp3
```

### 2. C/C++程序集成

#### 基本使用示例

```c
#include <stdio.h>
#include <lame.h>

int main() {
    lame_global_flags *gfp;
    int ret_code;
    
    // 初始化LAME编码器
    gfp = lame_init();
    if (gfp == NULL) {
        printf("LAME初始化失败\n");
        return -1;
    }
    
    // 设置编码参数
    lame_set_in_samplerate(gfp, 44100);    // 输入采样率
    lame_set_VBR(gfp, vbr_default);        // VBR模式
    lame_set_VBR_quality(gfp, 2);          // VBR质量(0-9)
    
    // 初始化编码器参数
    ret_code = lame_init_params(gfp);
    if (ret_code < 0) {
        printf("参数初始化失败\n");
        lame_close(gfp);
        return -1;
    }
    
    // 进行编码处理...
    // 使用lame_encode_buffer()进行实际编码
    
    // 清理资源
    lame_close(gfp);
    return 0;
}
```

#### 编译链接

**方法1：使用Makefile**
```makefile
CC = gcc
CFLAGS = -Wall -O2
LAME_PREFIX = /path/to/lame/build/install

INCLUDES = -I$(LAME_PREFIX)/include
LIBS = -L$(LAME_PREFIX)/lib -lmp3lame -lm

your_program: your_program.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)
```

**方法2：直接编译**
```bash
# 编译
gcc -Wall -O2 -Ibuild/install/include -Lbuild/install/lib \
    -o your_program your_program.c -lmp3lame -lm

# 运行（设置库路径）
export DYLD_LIBRARY_PATH=build/install/lib:$DYLD_LIBRARY_PATH  # macOS
export LD_LIBRARY_PATH=build/install/lib:$LD_LIBRARY_PATH      # Linux
./your_program
```

**方法3：CMake集成**
```cmake
# CMakeLists.txt
find_path(LAME_INCLUDE_DIR lame.h PATHS /path/to/lame/build/install/include)
find_library(LAME_LIBRARY mp3lame PATHS /path/to/lame/build/install/lib)

target_include_directories(your_target PRIVATE ${LAME_INCLUDE_DIR})
target_link_libraries(your_target PRIVATE ${LAME_LIBRARY})
```

### 3. Android项目集成

#### Android.mk方式
```makefile
LOCAL_PATH := $(call my-dir)

# 预编译库
include $(CLEAR_VARS)
LOCAL_MODULE := mp3lame
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libmp3lame.so
include $(PREBUILT_SHARED_LIBRARY)

# 你的应用
include $(CLEAR_VARS)
LOCAL_MODULE := your_app
LOCAL_SRC_FILES := your_app.c
LOCAL_SHARED_LIBRARIES := mp3lame
include $(BUILD_SHARED_LIBRARY)
```

#### CMakeLists.txt方式
```cmake
# 添加预编译库
add_library(mp3lame SHARED IMPORTED)
set_target_properties(mp3lame PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI}/libmp3lame.so
)

# 链接到你的目标
target_link_libraries(your_app mp3lame)
```

### 4. 常用编码参数

```c
// CBR (固定比特率)
lame_set_VBR(gfp, vbr_off);
lame_set_brate(gfp, 128);  // 128 kbps

// ABR (平均比特率)
lame_set_VBR(gfp, vbr_abr);
lame_set_VBR_mean_bitrate_kbps(gfp, 192);

// VBR (可变比特率)
lame_set_VBR(gfp, vbr_default);
lame_set_VBR_quality(gfp, 2);  // 0=最高质量, 9=最小文件

// 其他常用参数
lame_set_in_samplerate(gfp, 44100);     // 输入采样率
lame_set_num_channels(gfp, 2);          // 声道数
lame_set_quality(gfp, 2);               // 编码质量(0-9)
lame_set_mode(gfp, JOINT_STEREO);       // 立体声模式
```

### 5. 完整编码示例

项目中包含了完整的使用示例：
- `example_usage.c` - 基本API使用示例
- `Makefile.example` - 编译配置示例

运行示例：
```bash
# 编译示例
make -f Makefile.example

# 运行示例
make -f Makefile.example run
```

### 6. 系统安装

如果需要系统级安装：

```bash
# 复制到系统目录
sudo cp build/install/lib/libmp3lame.* /usr/local/lib/
sudo cp build/install/include/lame.h /usr/local/include/
sudo cp build/install/bin/lame /usr/local/bin/

# 更新库缓存（Linux）
sudo ldconfig

# 之后可以直接使用
gcc your_program.c -lmp3lame -lm
```

## 性能优化

### 架构特定优化

#### ARM NEON优化
ARMv8构建自动启用NEON SIMD指令：
```cmake
-DHAVE_NEON=ON
-march=armv8-a
```

#### x86 SSE优化
x86_64构建包含向量化优化：
```cmake
# 自动检测xmm_quantize_sub.c
```

### 编译优化选项
```bash
# 最大优化
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-O3 -DNDEBUG"

# 体积优化
cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel

# 调试信息
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

## 故障排除

### 常见问题

#### 1. CMake版本过低
```
CMake Error: CMake 3.16 or higher is required
```
**解决**: 升级CMake到3.16+

#### 2. 找不到交叉编译工具链
```
-- The C compiler identification is unknown
```
**解决**: 安装对应的交叉编译工具链

#### 3. Android NDK路径错误
```
ERROR: NDK路径不存在
```
**解决**: 检查NDK路径并使用正确的路径

#### 4. 构建失败
```
error: 构建失败
```
**解决**: 
1. 检查依赖是否安装完整
2. 使用`--clean`选项清理构建目录
3. 查看详细错误信息

### 调试模式
```bash
# 开启详细输出
./build_lame.sh --build-type Debug

# CMake详细输出
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
```

## 与原有构建系统的兼容性

新的CMake系统与原有autotools系统**并存**，您可以选择使用：

### 使用新系统（推荐）
```bash
./build_lame.sh
```

### 使用传统系统
```bash
./configure --enable-nasm
make
make install
```

### 使用平台特定Makefile
```bash
# Unix平台
make -f Makefile.unix

# Windows MSVC
nmake -f Makefile.MSVC
```

## 贡献和反馈

如果您在使用过程中遇到问题或有改进建议，请：

1. 检查本文档中的故障排除部分
2. 查看CMakeLists.txt中的配置选项
3. 提交问题报告时请包含：
   - 目标平台和架构
   - 完整的错误信息
   - 构建环境信息

## 参考资料

- [LAME官方网站](http://lame.sourceforge.net/)
- [CMake官方文档](https://cmake.org/documentation/)
- [Android NDK构建系统](https://developer.android.com/ndk/guides/cmake)
- [FFmpeg交叉编译参考](https://blog.csdn.net/T_T233333333/article/details/147515123)

---

**注意**: 这是LAME项目的非官方现代化构建系统，旨在提供更好的跨平台支持和开发体验。 