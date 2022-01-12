/**
 *
 *  Filename:      das_rlimit.c
 *
 *  Description:   Message rate limit control
 *
 *  Created by strawmanbobi 2022-01-10
 *
 *  Copyright (c) 2022 Aliyun-IoT
 *
 **/

#include <stdint.h>
#include <stdbool.h>

#include "das_rlimit.h"
#include "das_configure.h"

#if (DAS_RATE_LIMIT_ENABLE)

static int rl_tps = 1;
static uint64_t rl_begin_time = 0;
static uint64_t rl_used_ticket = 0;

static void rate_limiter_reset(uint64_t now_s) {
    rl_used_ticket = 0;
    rl_begin_time = now_s;
}

void rate_limiter_init(uint64_t tps, uint64_t now_s) {
    rl_tps = tps;
    rate_limiter_reset(now_s);
}

bool rate_limiter_pass(uint64_t now_s) {
#if defined(_NO_RATE_LIMIT_)
    return true;
#else
    bool pass = false;

    // not used this because only reporter thread will call this file
    //    pthread_mutex_lock(&g_lock_ticket);
    if ((now_s - rl_begin_time) >= (rl_used_ticket * (1000 / rl_tps))) {
        rl_used_ticket++;
        pass = true;
    }

    // reset each 1000 ms
    if (now_s - rl_begin_time > 1000) {
        rate_limiter_reset(now_s);
        if (pass) { // because we give the ticket this time
            rl_used_ticket++;
        }
    }
    //    pthread_mutex_unlock(&g_lock_ticket);

    return pass;
#endif
}

#endif /* DAS_RATE_LIMIT_ENABLE */