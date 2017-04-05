/* enables use of clock_gettime() on OS X, based on code by Mathias Panzenböck */

#include <sys/types.h>
#include <sys/_types/_timespec.h>
#include <mach/mach.h>
#include <mach/clock.h>

#ifndef mach_time_h
#define mach_time_h

#define TIMER_ABSTIME -1
#define CLOCK_REALTIME CALENDAR_CLOCK
#define CLOCK_MONOTONIC SYSTEM_CLOCK

typedef int clockid_t;

int clock_gettime(clockid_t clk_id, struct timespec *tp);

#endif
