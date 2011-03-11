/* Really simple benchmarking tool. Surround sections of code to benchmark with
   TICK(); ... TOCK(); and look at the benchmark_total_time variable to get the
   total number of microseconds elapsed between TICK and TOCK calls. You must
   call BENCHMARK_INIT() at the beginning of your program. */
#ifndef __BENCHMARK_H
#define __BENCHMARK_H

#include <time.h>

static clock_t __benchmark_h_time;
static int benchmark_total_time = 0;

#define BENCHMARK_INIT() benchmark_total_time = 0;
#define TICK() __benchmark_h_time = clock();
#define TOCK() benchmark_total_time += clock() - __benchmark_h_time;

#endif
