cmake_minimum_required(VERSION 3.16)

# 项目信息
project(lame
    VERSION 3.99.5
    DESCRIPTION "LAME MP3 Encoder"
    LANGUAGES C CXX
)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 构建选项
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(WITH_DECODER "Include MP3 decoder" ON)
option(WITH_FRONTEND "Build command-line frontend" ON)
option(WITH_MP3X "Build MP3 analysis tool" OFF)
option(WITH_MP3RTP "Build MP3 RTP streaming tool" OFF)

# 平台检测
if(CMAKE_SYSTEM_NAME MATCHES "Android")
    set(ANDROID_BUILD ON)
endif()

# 架构检测
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
    set(TARGET_ARCH "arm64")
    set(HAVE_ARM ON)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    set(TARGET_ARCH "arm")
    set(HAVE_ARM ON)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86|x86")
    set(TARGET_ARCH "x86")
    set(HAVE_X86 ON)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    set(TARGET_ARCH "x86_64")
    set(HAVE_X86_64 ON)
endif()

# 编译器特定设置
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3")
    # 添加标准库兼容性标志
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
    
    # macOS特定修复
    if(APPLE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit-function-declaration")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_DARWIN_C_SOURCE")
    endif()
    
    if(HAVE_ARM)
        if(TARGET_ARCH STREQUAL "arm64")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a")
        else()
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfpu=neon")
        endif()
    endif()
endif()

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libmp3lame
    ${CMAKE_CURRENT_SOURCE_DIR}/mpglib
    ${CMAKE_CURRENT_SOURCE_DIR}  # 包含fix_headers.h
    ${CMAKE_CURRENT_BINARY_DIR}
)

# 配置头文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/configMS.h
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
    COPYONLY
)

# 添加必要的编译器定义
add_definitions(-DHAVE_CONFIG_H)
add_definitions(-D_GNU_SOURCE)

# 确保包含标准头文件
if(NOT WIN32)
    add_definitions(-D_POSIX_C_SOURCE=200809L)
    add_definitions(-D_DEFAULT_SOURCE)
endif()

# 数学库
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    set(MATH_LIB m)
endif()

# 核心库源文件
set(LIBMP3LAME_SOURCES
    libmp3lame/VbrTag.c
    libmp3lame/bitstream.c
    libmp3lame/encoder.c
    libmp3lame/fft.c
    libmp3lame/gain_analysis.c
    libmp3lame/id3tag.c
    libmp3lame/lame.c
    libmp3lame/newmdct.c
    libmp3lame/presets.c
    libmp3lame/psymodel.c
    libmp3lame/quantize.c
    libmp3lame/quantize_pvt.c
    libmp3lame/reservoir.c
    libmp3lame/set_get.c
    libmp3lame/tables.c
    libmp3lame/takehiro.c
    libmp3lame/util.c
    libmp3lame/vbrquantize.c
    libmp3lame/version.c
)

# 条件性添加解码器接口
if(WITH_DECODER)
    list(APPEND LIBMP3LAME_SOURCES libmp3lame/mpglib_interface.c)
    add_definitions(-DHAVE_MPGLIB)
endif()

# MPGLIB解码器库
if(WITH_DECODER)
    set(MPGLIB_SOURCES
        mpglib/common.c
        mpglib/dct64_i386.c
        mpglib/decode_i386.c
        mpglib/interface.c
        mpglib/layer1.c
        mpglib/layer2.c
        mpglib/layer3.c
        mpglib/tabinit.c
    )
    
    add_library(mpgdecoder STATIC ${MPGLIB_SOURCES})
    
    # 为mpglib源文件添加特殊的编译标志
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(mpgdecoder PRIVATE 
            -include ${CMAKE_CURRENT_SOURCE_DIR}/fix_headers.h
        )
    endif()
endif()

# 汇编优化（针对x86架构）
if(HAVE_X86 OR HAVE_X86_64)
    # 这里可以添加x86汇编优化文件
    # 注意：原项目的i386汇编代码需要NASM
endif()

# ARM NEON优化
if(HAVE_ARM AND TARGET_ARCH STREQUAL "arm64")
    # 可以在这里添加ARM NEON优化代码
    add_definitions(-DHAVE_NEON)
endif()

# 向量化优化
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libmp3lame/vector/xmm_quantize_sub.c)
    set(VECTOR_SOURCES libmp3lame/vector/xmm_quantize_sub.c)
    add_library(lamevector STATIC ${VECTOR_SOURCES})
    
    # 为向量化源文件添加头文件修复
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(lamevector PRIVATE 
            -include ${CMAKE_CURRENT_SOURCE_DIR}/fix_headers.h
        )
    endif()
endif()

# 创建libmp3lame库
add_library(mp3lame ${LIBMP3LAME_SOURCES})

# 为主库添加头文件修复
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mp3lame PRIVATE 
        -include ${CMAKE_CURRENT_SOURCE_DIR}/fix_headers.h
    )
endif()

# 链接依赖
target_link_libraries(mp3lame PRIVATE ${MATH_LIB})
if(WITH_DECODER AND TARGET mpgdecoder)
    target_link_libraries(mp3lame PRIVATE mpgdecoder)
endif()
if(TARGET lamevector)
    target_link_libraries(mp3lame PRIVATE lamevector)
endif()

# 设置库版本
set_target_properties(mp3lame PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    PUBLIC_HEADER "include/lame.h"
)

# 前端工具
if(WITH_FRONTEND)
    set(FRONTEND_SOURCES
        frontend/main.c
        frontend/lame_main.c
        frontend/brhist.c
        frontend/console.c
        frontend/get_audio.c
        frontend/lametime.c
        frontend/parse.c
        frontend/timestatus.c
    )
    
    add_executable(lame ${FRONTEND_SOURCES})
    target_link_libraries(lame PRIVATE mp3lame)
    
    # 为前端工具添加头文件修复
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(lame PRIVATE 
            -include ${CMAKE_CURRENT_SOURCE_DIR}/fix_headers.h
        )
    endif()
    
    # 链接系统库
    if(WIN32)
        target_link_libraries(lame PRIVATE winmm)
    endif()
endif()

# MP3X分析工具
if(WITH_MP3X)
    add_executable(mp3x frontend/mp3x.c)
    target_link_libraries(mp3x PRIVATE mp3lame)
    
    # 为MP3X添加头文件修复
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(mp3x PRIVATE 
            -include ${CMAKE_CURRENT_SOURCE_DIR}/fix_headers.h
        )
    endif()
endif()

# MP3RTP流媒体工具
if(WITH_MP3RTP)
    set(MP3RTP_SOURCES
        frontend/mp3rtp.c
        frontend/rtp.c
    )
    add_executable(mp3rtp ${MP3RTP_SOURCES})
    target_link_libraries(mp3rtp PRIVATE mp3lame)
    
    # 为MP3RTP添加头文件修复
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(mp3rtp PRIVATE 
            -include ${CMAKE_CURRENT_SOURCE_DIR}/fix_headers.h
        )
    endif()
endif()

# 安装配置
include(GNUInstallDirs)

install(TARGETS mp3lame
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(WITH_FRONTEND)
    install(TARGETS lame
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(WITH_MP3X)
    install(TARGETS mp3x
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(WITH_MP3RTP)
    install(TARGETS mp3rtp
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# 安装手册页
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/doc/man/lame.1)
    install(FILES doc/man/lame.1
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
    )
endif()

# 添加子目录处理（如果需要）
# add_subdirectory(tests)

# 显示构建信息
message(STATUS "LAME ${PROJECT_VERSION} 构建配置:")
message(STATUS "  目标架构: ${TARGET_ARCH}")
message(STATUS "  构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "  共享库: ${BUILD_SHARED_LIBS}")
message(STATUS "  包含解码器: ${WITH_DECODER}")
message(STATUS "  构建前端: ${WITH_FRONTEND}")
if(ANDROID_BUILD)
    message(STATUS "  Android构建: 是")
    message(STATUS "  Android ABI: ${ANDROID_ABI}")
endif() 