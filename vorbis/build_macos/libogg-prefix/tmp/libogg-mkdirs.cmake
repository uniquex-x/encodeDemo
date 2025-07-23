# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/libogg-1.3.6")
  file(MAKE_DIRECTORY "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/libogg-1.3.6")
endif()
file(MAKE_DIRECTORY
  "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/libogg-prefix/src/libogg-build"
  "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/install_macos"
  "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/libogg-prefix/tmp"
  "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/libogg-prefix/src/libogg-stamp"
  "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/libogg-prefix/src"
  "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/libogg-prefix/src/libogg-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/libogg-prefix/src/libogg-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/libogg-prefix/src/libogg-stamp${cfgdir}") # cfgdir has leading slash
endif()
