cmake_minimum_required(VERSION 3.10)
project(AudioencoderManager)

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
    ResourceMonitor.cpp
    flacEncoder.cpp
    mp3Encoder.cpp
    vorbisEncoder.cpp
    AudioQualityAnalyzer.cpp
    AudioFileLoader.cpp
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

# 创建音质分析测试程序
add_executable(audio_quality_test 
    audio_quality_test.cpp
    AudioQualityAnalyzer.cpp
    AudioFileLoader.cpp
)

# 链接音质分析测试程序
target_link_libraries(audio_quality_test
    vorbisfile
    vorbis
    ogg
)

# 创建音频文件加载测试程序
add_executable(audio_file_test
    audio_file_test.cpp
    AudioFileLoader.cpp
)

# 链接音频文件加载测试程序
target_link_libraries(audio_file_test
    vorbisfile
    vorbis
    ogg
)