cmake_minimum_required(VERSION 3.10)
project(AudioEncoderTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置包含目录
include_directories(../include/flac)
include_directories(../include/lame)
include_directories(../include/vorbis)

# 设置库文件目录
link_directories(../libs/flac)
link_directories(../libs/lame)
link_directories(../libs/vorbis)

# 源文件
set(SOURCES
    main.cpp
    EncoderManager.cpp
    flacEncoder.cpp
    mp3Encoder.cpp
    vorbisEncoder.cpp
)

# 导出编译命令
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 创建可执行文件
add_executable(audio_encoder_test ${SOURCES})

# 链接库文件
target_link_libraries(audio_encoder_test
    FLAC
    FLAC++
    mp3lame
    vorbis
    vorbisenc
    vorbisfile
    ogg
)

# macOS特定设置
if(APPLE)
    set_target_properties(audio_encoder_test PROPERTIES
        INSTALL_RPATH "@loader_path/../libs/flac;@loader_path/../libs/lame;@loader_path/../libs/vorbis"
    )
endif()

# 复制测试音频文件到构建目录
file(COPY samples DESTINATION ${CMAKE_CURRENT_BINARY_DIR})