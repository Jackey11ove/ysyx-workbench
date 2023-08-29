#ifndef __TIMER_H__
#define __TIMER_H__

#include <common.h>

struct timeval {
    long         tv_sec;      /* seconds */
    long         tv_usec;     /* microseconds */
};

int gettimeofday(struct timeval *tv);

#endif