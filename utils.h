#ifndef _UTILS_H
#define _UTILS_H

#include <sys/syscall.h>
#include <stdio.h>

#define MAX_TIMERS 20
char* _timer_names[MAX_TIMERS];

#ifdef __APPLE__
#define CREATE_TIMER(t, index) static int t = index
#else
#define CREATE_TIMER(t) static int t = __COUNTER__
#endif

#define FAIL(fmt, ...)                                                \
  do {printf("%s:%d FAILURE: " fmt, __FILE__, __LINE__, __VA_ARGS__); \
      exit(-1);} while (0);

void init_rnd(unsigned int rseed);
int get_int_rnd(int range);
double get_double_rnd(double min, double max);
double _get_current_time(void);
void timer_clear(int n);
void timer_start(int n);
void timer_stop(int n);
double timer_read(int n);
void mfprintf(FILE* f, int num_args, ...);
#define mprintf(num_args, ...) mfprintf(stdout, num_args, __VA_ARGS__)

#endif
