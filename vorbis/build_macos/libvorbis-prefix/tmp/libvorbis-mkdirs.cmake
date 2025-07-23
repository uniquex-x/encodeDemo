# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/libvorbis-1.3.7")
  file(MAKE_DIRECTORY "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/libvorbis-1.3.7")
endif()
file(MAKE_DIRECTORY
  "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/libvorbis-prefix/src/libvorbis-build"
  "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/install_macos"
  "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/libvorbis-prefix/tmp"
  "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/libvorbis-prefix/src/libvorbis-stamp"
  "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/libvorbis-prefix/src"
  "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/libvorbis-prefix/src/libvorbis-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/libvorbis-prefix/src/libvorbis-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/ljh/音频编码算法研究/源码/compileScript/vorbis/build_macos/libvorbis-prefix/src/libvorbis-stamp${cfgdir}") # cfgdir has leading slash
endif()
