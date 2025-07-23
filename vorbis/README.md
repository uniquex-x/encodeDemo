# Ogg Vorbis 音频库交叉编译脚本

本项目提供了一套完整的 Ogg Vorbis 音频库交叉编译解决方案，支持 Android、macOS、Linux、Windows 等多平台构建。采用现代化的 CMake + NDK 工具链方法，避免了传统手工脚本的复杂性和兼容性问题。

## 📋 特性

- ✅ **现代化工具链**: 使用 NDK r21+ 的 `android.toolchain.cmake`
- ✅ **多架构支持**: Android arm64-v8a、armeabi-v7a 架构
- ✅ **多平台支持**: Android、macOS、Linux、Windows
- ✅ **自动化构建**: 先构建 libogg，再构建 libvorbis
- ✅ **灵活配置**: 支持动态库/静态库、Debug/Release 模式
- ✅ **智能检测**: 自动检测平台和工具链
- ✅ **友好界面**: 彩色输出和详细的构建信息

## 📁 项目结构

```
vorbis/
├── CMakeLists.txt          # 主CMake配置文件
├── build_vorbis.sh         # 主构建脚本
├── quick_build.sh          # 快速构建脚本（菜单模式）
├── README.md               # 本文档
├── libogg-1.3.6/          # libogg 源码
└── libvorbis-1.3.7/       # libvorbis 源码
```

## 🚀 快速开始

### 方法一：使用快速构建脚本（推荐新手）

```bash
./quick_build.sh
```

这将启动一个交互式菜单，引导您完成构建过程。

### 方法二：使用命令行（推荐高级用户）

#### Android 构建

```bash
# 单个架构
./build_vorbis.sh --platform android --android-abi arm64-v8a --android-ndk /path/to/ndk

# 所有 Android 架构
./build_vorbis.sh --platform android --android-abi all --android-ndk /path/to/ndk
```

#### 原生平台构建

```bash
# macOS
./build_vorbis.sh --platform macos

# Linux
./build_vorbis.sh --platform linux

# 自动检测平台
./build_vorbis.sh
```

## 📖 详细使用说明

### 命令行参数

| 参数 | 说明 | 可选值 | 默认值 |
|------|------|--------|--------|
| `--platform` | 目标平台 | `android`, `macos`, `linux`, `windows` | 自动检测 |
| `--android-abi` | Android架构 | `arm64-v8a`, `armeabi-v7a`, `all` | `arm64-v8a` |
| `--android-ndk` | Android NDK路径 | 有效的NDK路径 | 环境变量`$ANDROID_NDK` |
| `--build-type` | 构建类型 | `Release`, `Debug` | `Release` |
| `--shared` | 构建动态库 | - | 默认 |
| `--static` | 构建静态库 | - | - |
| `--clean` | 清理构建产物 | - | - |
| `--help` | 显示帮助信息 | - | - |

### 环境变量

| 变量名 | 说明 | 示例 |
|--------|------|------|
| `ANDROID_NDK` | Android NDK路径 | `/Users/username/android-ndk-r25c` |

### 构建示例

#### 1. Android 开发

```bash
# 设置NDK环境变量（可选）
export ANDROID_NDK="/Users/username/android-ndk-r25c"

# 构建 arm64-v8a 动态库
./build_vorbis.sh --platform android --android-abi arm64-v8a

# 构建 armeabi-v7a 静态库
./build_vorbis.sh --platform android --android-abi armeabi-v7a --static

# 构建所有Android架构
./build_vorbis.sh --platform android --android-abi all
```

#### 2. 桌面应用开发

```bash
# macOS 开发
./build_vorbis.sh --platform macos

# Linux 开发
./build_vorbis.sh --platform linux --build-type Debug

# 清理所有构建产物
./build_vorbis.sh --clean
```

## 📦 构建产物

构建完成后，库文件将安装到以下目录：

```
install_<platform>_<arch>/
├── lib/                    # 库文件
│   ├── libogg.*           # Ogg 动态库 (.so/.dylib/.dll)
│   ├── libvorbis.*        # Vorbis 核心库
│   ├── libvorbisenc.*     # Vorbis 编码库  
│   ├── libvorbisfile.*    # Vorbis 文件操作库
│   └── pkgconfig/         # pkg-config 配置文件
└── include/               # 头文件
    ├── ogg/              # Ogg 头文件
    └── vorbis/           # Vorbis 头文件
```

### 平台库文件扩展名

- **Linux**: `.so` (shared object)
- **macOS**: `.dylib` (dynamic library) 
- **Windows**: `.dll` (dynamic link library)
- **静态库**: `.a` (archive) - 所有平台通用

### 平台特定的目录名称

- Android arm64-v8a: `install_android_arm64-v8a/`
- Android armeabi-v7a: `install_android_armeabi-v7a/`
- macOS: `install_macos/`
- Linux: `install_linux/`
- Windows: `install_windows/`

## 🔧 集成到项目

### Android Studio / CMake

```cmake
# 设置库路径 (以Android为例)
set(VORBIS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/path/to/install_android_arm64-v8a)

# 添加库 (Android使用.so扩展名)
add_library(ogg SHARED IMPORTED)
set_target_properties(ogg PROPERTIES IMPORTED_LOCATION ${VORBIS_ROOT}/lib/libogg.so)

add_library(vorbis SHARED IMPORTED)
set_target_properties(vorbis PROPERTIES IMPORTED_LOCATION ${VORBIS_ROOT}/lib/libvorbis.so)

add_library(vorbisenc SHARED IMPORTED)
set_target_properties(vorbisenc PROPERTIES IMPORTED_LOCATION ${VORBIS_ROOT}/lib/libvorbisenc.so)

add_library(vorbisfile SHARED IMPORTED)
set_target_properties(vorbisfile PROPERTIES IMPORTED_LOCATION ${VORBIS_ROOT}/lib/libvorbisfile.so)

# 设置头文件路径
target_include_directories(your_target PRIVATE 
    ${VORBIS_ROOT}/include
)

# 链接库
target_link_libraries(your_target 
    ogg vorbis vorbisenc vorbisfile
)
```

#### macOS 集成示例

```cmake
# macOS使用.dylib扩展名
set(VORBIS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/path/to/install_macos)

# 添加库
add_library(ogg SHARED IMPORTED)
set_target_properties(ogg PROPERTIES IMPORTED_LOCATION ${VORBIS_ROOT}/lib/libogg.dylib)

add_library(vorbis SHARED IMPORTED)
set_target_properties(vorbis PROPERTIES IMPORTED_LOCATION ${VORBIS_ROOT}/lib/libvorbis.dylib)

add_library(vorbisenc SHARED IMPORTED)
set_target_properties(vorbisenc PROPERTIES IMPORTED_LOCATION ${VORBIS_ROOT}/lib/libvorbisenc.dylib)

add_library(vorbisfile SHARED IMPORTED)
set_target_properties(vorbisfile PROPERTIES IMPORTED_LOCATION ${VORBIS_ROOT}/lib/libvorbisfile.dylib)

# 其余配置同上...
```

### pkg-config 使用

```bash
# 设置 PKG_CONFIG_PATH
export PKG_CONFIG_PATH=/path/to/install_macos/lib/pkgconfig:$PKG_CONFIG_PATH

# 编译时使用
gcc $(pkg-config --cflags vorbis vorbisenc vorbisfile) your_app.c \
    $(pkg-config --libs vorbis vorbisenc vorbisfile) -o your_app
```

## 🛠 系统要求

### 必需工具

- **CMake** >= 3.18.1
- **构建工具**: make 或 ninja
- **编译器**: GCC、Clang 或 MSVC

### Android 开发额外要求

- **Android NDK** >= r21
- 支持的目标API级别: >= 21 (Android 5.0)

### 各平台具体要求

#### macOS
```bash
# 安装 Xcode Command Line Tools
xcode-select --install

# 使用 Homebrew 安装 CMake（可选）
brew install cmake
```

#### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake
```

#### Windows
- Visual Studio 2019+ 或 Visual Studio Build Tools
- CMake（可通过 Visual Studio Installer 安装）

## 🐛 常见问题

### 1. NDK 路径问题

**问题**: `无效的NDK路径，找不到android.toolchain.cmake`

**解决方案**:
- 确保使用 NDK r21 或更新版本
- 检查路径是否正确：`$NDK_PATH/build/cmake/android.toolchain.cmake` 应该存在

### 2. 构建失败

**问题**: `cmake配置失败`

**解决方案**:
```bash
# 清理构建缓存
./build_vorbis.sh --clean

# 检查CMake版本
cmake --version

# 确保满足最低版本要求 (3.18.1+)
```

### 3. 库文件缺失

**问题**: 构建完成但找不到库文件

**解决方案**:
- 检查构建日志中的错误信息
- 确保有足够的磁盘空间
- 验证安装目录权限

### 4. 交叉编译配置错误

**问题**: Android 构建产生的是 macOS 库

**解决方案**:
- 确保正确设置了 `--platform android` 参数
- 验证 NDK 工具链文件完整性

## 🔍 调试和日志

### 启用详细日志

构建脚本默认启用详细日志。如需查看更多调试信息：

```bash
# 查看CMake配置日志
cat build_android_arm64-v8a/CMakeOutput.log

# 查看构建错误日志
cat build_android_arm64-v8a/CMakeError.log
```

### 手动调试构建

```bash
# 进入构建目录
cd build_android_arm64-v8a

# 手动执行CMake配置
cmake .. -DBUILD_ANDROID=ON -DANDROID_ABI=arm64-v8a -DANDROID_NDK=/path/to/ndk

# 手动构建
make VERBOSE=1
```

## 📚 参考资料

- [Android NDK CMake 指南](https://developer.android.com/ndk/guides/cmake)
- [Ogg 官方文档](https://xiph.org/ogg/doc/)
- [Vorbis 官方文档](https://xiph.org/vorbis/doc/)
- [CMake ExternalProject 文档](https://cmake.org/cmake/help/latest/module/ExternalProject.html)

## 🤝 贡献

欢迎提交 Issue 和 Pull Request 来改进这个构建脚本！

## 📄 许可证

本构建脚本采用 MIT 许可证。Ogg 和 Vorbis 库本身采用 BSD 许可证。

---

**提示**: 首次使用建议运行 `./quick_build.sh` 来熟悉构建流程，有经验的开发者可以直接使用 `./build_vorbis.sh` 命令行工具。 