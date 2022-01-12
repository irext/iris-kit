/**
 *
 *  Filename:      das_rlimit.h
 *
 *  Description:   Message rate limit control
 *
 *  Created by strawmanbobi 2022-01-10
 *
 *  Copyright (c) 2022 Aliyun-IoT
 *
 **/

#ifndef __DAS_RLIMIT_H__
#define __DAS_RLIMIT_H__

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

void rate_limiter_init(uint64_t tps, uint64_t now_s);

bool rate_limiter_pass(uint64_t now_s);

#if defined(__cplusplus)
}
#endif

#endif