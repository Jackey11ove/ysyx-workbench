#include <am.h>
#define DEVICE_BASE 0xa0000000
#define RTC_ADDR (DEVICE_BASE + 0x0000048)

static inline uint32_t ind(uintptr_t addr) { return *(volatile uint64_t *)addr; }

void __am_timer_init() {
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uptime->us = ind(RTC_ADDR);
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
