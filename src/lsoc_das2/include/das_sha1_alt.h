/**
 *
 *  Filename:      sha1_alt.h
 *
 *  Description:   Yet another implementation of SHA1 algorithms
 *
 *  Created by strawmanbobi 2022-01-10
 *
 *  Copyright (c) 2022 Aliyun-IoT
 *
 **/

#ifndef __DAS_SHA1_ALT_H__
#define __DAS_SHA1_ALT_H__

#include "das_platform.h"

typedef struct _das_sha1_context {
    void *context;
    uint32_t size;
} das_sha1_context_t;

#endif /* __DAS_SHA1_ALT_H__ */
