#include <common.h>
#include <timer.h>

int gettimeofday(struct timeval *tv){
    size_t us = io_read(AM_TIMER_UPTIME).us;
    if(tv != NULL){
      tv->tv_sec = us / 1000000;
      tv->tv_usec = us % 1000000;
      return 0;
    }else{
      return -1;
    }
}