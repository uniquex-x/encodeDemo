cmake_minimum_required(VERSION 3.18.1)

project(VorbisAudioLibs 
    VERSION 1.0.0
    DESCRIPTION "Ogg Vorbis Audio Libraries Cross-Compilation"
    LANGUAGES C
)

# 包含必要的模块
include(ExternalProject)
include(GNUInstallDirs)

# ====== 配置选项 ======
option(BUILD_ANDROID "Build for Android" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Android架构选项
set(ANDROID_ABI_LIST "arm64-v8a;armeabi-v7a" CACHE STRING "Android ABIs to build")

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# ====== 平台检测和配置 ======
if(BUILD_ANDROID)
    if(NOT ANDROID_NDK)
        message(FATAL_ERROR "ANDROID_NDK must be set when BUILD_ANDROID=ON")
    endif()
    
    if(NOT ANDROID_ABI)
        set(ANDROID_ABI "arm64-v8a" CACHE STRING "Android ABI" FORCE)
    endif()
    
    message(STATUS "Building for Android ${ANDROID_ABI}")
    set(PLATFORM_SUFFIX "_android_${ANDROID_ABI}")
    set(INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/install${PLATFORM_SUFFIX}")
else()
    # 原生平台构建
    if(APPLE)
        set(PLATFORM_SUFFIX "_macos")
    elseif(WIN32)
        set(PLATFORM_SUFFIX "_windows")
    else()
        set(PLATFORM_SUFFIX "_linux")
    endif()
    
    message(STATUS "Building for native platform: ${CMAKE_SYSTEM_NAME}")
    set(INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/install${PLATFORM_SUFFIX}")
endif()

# ====== 创建安装目录 ======
file(MAKE_DIRECTORY ${INSTALL_PREFIX})
file(MAKE_DIRECTORY ${INSTALL_PREFIX}/lib)
file(MAKE_DIRECTORY ${INSTALL_PREFIX}/include)

# ====== 通用编译参数 ======
set(COMMON_C_FLAGS "-fPIC")
set(COMMON_CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
    -DCMAKE_C_FLAGS=${COMMON_C_FLAGS}
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
)

# ====== Android特定配置 ======
if(BUILD_ANDROID)
    list(APPEND COMMON_CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
        -DANDROID_ABI=${ANDROID_ABI}
        -DANDROID_PLATFORM=android-21
        -DANDROID_STL=c++_shared
        -DANDROID_CPP_FEATURES=rtti\ exceptions
    )
    
    # Android特定编译标志
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        list(APPEND COMMON_CMAKE_ARGS -DANDROID_ARM_NEON=TRUE)
    elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
        list(APPEND COMMON_CMAKE_ARGS -DANDROID_ARM_NEON=TRUE)
    endif()
endif()

# ====== 构建libogg ======
ExternalProject_Add(
    libogg
    SOURCE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/libogg-1.3.6
    CMAKE_ARGS      ${COMMON_CMAKE_ARGS}
                    -DINSTALL_DOCS=OFF
                    -DINSTALL_PKG_CONFIG_MODULE=ON
                    -DINSTALL_CMAKE_PACKAGE_MODULE=ON
    BUILD_IN_SOURCE FALSE
    INSTALL_DIR     ${INSTALL_PREFIX}
    LOG_CONFIGURE   TRUE
    LOG_BUILD       TRUE
    LOG_INSTALL     TRUE
)

# ====== 构建libvorbis ======
# 设置vorbis查找ogg的路径
set(VORBIS_CMAKE_ARGS ${COMMON_CMAKE_ARGS})
list(APPEND VORBIS_CMAKE_ARGS
    -DINSTALL_CMAKE_PACKAGE_MODULE=ON
    -DOgg_ROOT=${INSTALL_PREFIX}
    -DCMAKE_PREFIX_PATH=${INSTALL_PREFIX}
    -DOGG_INCLUDE_DIRS=${INSTALL_PREFIX}/include
    -DOGG_LIBRARIES=${INSTALL_PREFIX}/lib
)

ExternalProject_Add(
    libvorbis
    SOURCE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/libvorbis-1.3.7
    CMAKE_ARGS      ${VORBIS_CMAKE_ARGS}
    BUILD_IN_SOURCE FALSE
    INSTALL_DIR     ${INSTALL_PREFIX}
    DEPENDS         libogg
    LOG_CONFIGURE   TRUE
    LOG_BUILD       TRUE
    LOG_INSTALL     TRUE
)

# ====== 创建便捷目标 ======
add_custom_target(ogg DEPENDS libogg)
add_custom_target(vorbis DEPENDS libvorbis)
add_custom_target(all_libs DEPENDS libogg libvorbis)

# ====== 安装后处理 ======
# 创建符号链接和拷贝文件的自定义命令
ExternalProject_Add_Step(
    libvorbis post_install
    COMMAND ${CMAKE_COMMAND} -E echo "=== Build completed ==="
    COMMAND ${CMAKE_COMMAND} -E echo "Libraries installed to: ${INSTALL_PREFIX}"
    COMMAND ${CMAKE_COMMAND} -E echo "Platform: ${PLATFORM_SUFFIX}"
    DEPENDEES install
    ALWAYS TRUE
)

# ====== 显示构建信息 ======
message(STATUS "==========================================")
message(STATUS "Vorbis Audio Libraries Build Configuration")
message(STATUS "==========================================")
message(STATUS "Build for Android: ${BUILD_ANDROID}")
if(BUILD_ANDROID)
    message(STATUS "Android NDK: ${ANDROID_NDK}")
    message(STATUS "Android ABI: ${ANDROID_ABI}")
    message(STATUS "Android Platform: android-21")
endif()
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Shared Libraries: ${BUILD_SHARED_LIBS}")
message(STATUS "Install Prefix: ${INSTALL_PREFIX}")
message(STATUS "Platform Suffix: ${PLATFORM_SUFFIX}")
message(STATUS "==========================================")

# ====== 多架构构建支持 ======
if(BUILD_ANDROID AND NOT DEFINED SINGLE_ABI_BUILD)
    # 为每个ABI创建单独的构建目标
    foreach(ABI IN LISTS ANDROID_ABI_LIST)
        if(NOT ABI STREQUAL ANDROID_ABI)
            ExternalProject_Add(
                vorbis_${ABI}
                SOURCE_DIR          ${CMAKE_CURRENT_SOURCE_DIR}
                CMAKE_ARGS          -DBUILD_ANDROID=ON
                                    -DANDROID_ABI=${ABI}
                                    -DANDROID_NDK=${ANDROID_NDK}
                                    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                                    -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
                                    -DSINGLE_ABI_BUILD=ON
                BINARY_DIR          ${CMAKE_CURRENT_BINARY_DIR}/build_${ABI}
                INSTALL_COMMAND     ""
                BUILD_ALWAYS        FALSE
                LOG_CONFIGURE       TRUE
                LOG_BUILD           TRUE
            )
        endif()
    endforeach()
    
    # 创建多架构构建目标
    add_custom_target(android_all_abis)
    foreach(ABI IN LISTS ANDROID_ABI_LIST)
        if(NOT ABI STREQUAL ANDROID_ABI)
            add_dependencies(android_all_abis vorbis_${ABI})
        endif()
    endforeach()
endif()

# ====== 清理目标 ======
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/install_android_arm64-v8a
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/install_android_armeabi-v7a
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/install_macos
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/install_linux
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/install_windows
    COMMENT "Cleaning all build artifacts"
) 