#include <rtthread.h>

#include <rtdbg.h>

struct timespec{
    long   tv_sec;
    long   tv_nsec;
};

int clock_gettime(clockid_t unused, struct timespec *tp)
{
  (void)unused;

  rt_tick_t millis = rt_tick_get_millisecond();

  tp->tv_sec = millis / 1000;
  tp->tv_nsec = (millis % 1000) * 1000;

  return 0;
}