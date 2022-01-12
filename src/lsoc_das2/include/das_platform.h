/**
 *
 *  Filename:      platform.h
 *
 *  Description:   OS specific definitions
 *
 *  Created by strawmanbobi 2022-01-10
 *
 *  Copyright (c) 2022 Aliyun-IoT
 *
 **/


#ifndef __DAS_PLATFORM_H__
#define __DAS_PLATFORM_H__

#include "das_configure.h"
#include "das_hardware.h"

/* Include all the system headers needed by nanopb. You will need the
 * definitions of the following:
 * - strlen, memcpy, memset functions
 * - [u]int_least8_t, uint_fast8_t, [u]int_least16_t, [u]int32_t, [u]int64_t
 * - size_t
 * - bool
 *
 * If you don't have the standard header files, you can instead provide
 * a custom header that defines or includes all this. In that case,
 * define PB_SYSTEM_HEADER to the path of this file.
 */

#ifdef DAS_PLATFORM_ALT
#include <das_platform_alt.h>
#else
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#ifdef DAS_FEATURE_MALLOC_ENABLED
#include <stdlib.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef i386
#define DAS_ARCH "x86"
#endif

#ifdef __x86_64__
#define DAS_ARCH "x86_64"
#endif

#ifdef __aarch64__
#define DAS_ARCH "aarch64c"
#endif

#ifdef __arm__p
#define DAS_ARCH "arm"
#endif

#ifdef __arm__
#define DAS_ARCH "arm"
#endif

#ifdef __mips__
#define DAS_ARCH "mips"
#endif

#ifdef __mips64
#define DAS_ARCH "mips64"
#endif

#ifdef __csky__
#define DAS_ARCH "csky"
#endif

#ifdef ESP8266
#define DAS_ARCH "arm"
#endif

#ifndef DAS_ARCH
#error "can not identify compiler's cpu arch."
#endif

#ifdef DAS_DEBUG
#define DAS_LOG(_f, ...)      printf("%s %s %d: " _f, "<das>", \
                                  __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define DAS_LOG(_f, ...) ; 
#endif

size_t das_hex2string(char *buffer, size_t buf_length, uint8_t *hex, size_t hex_length);

char *das_itoa(int value, char *string, unsigned int radix);

char** das_strsplit( const char* s, const char* delim, size_t* nb);

#ifdef __cplusplus
}
#endif

#endif /* __DAS_PLATFORM_H__ */
