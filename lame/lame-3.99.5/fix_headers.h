#ifndef LAME_FIX_HEADERS_H
#define LAME_FIX_HEADERS_H

/* 标准库头文件修复 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

/* POSIX和系统特定头文件 */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/* macOS/BSD特定修复 */
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <strings.h>  /* for bcopy */
#endif

/* 确保uint32_t等类型定义 */
#ifndef UINT32_MAX
#include <limits.h>
#endif

/* IEEE754浮点数类型定义 - 仅在未定义时定义 */
#if !defined(ieee754_float32_t) && !defined(HAVE_CONFIG_H)
typedef float ieee754_float32_t;
#endif

#if !defined(ieee754_float64_t) && !defined(HAVE_CONFIG_H)
typedef double ieee754_float64_t;
#endif

#endif /* LAME_FIX_HEADERS_H */ 