#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

#include "utils.h"

static unsigned int _rnd_seed = 29;
static double _elapsed_time[MAX_TIMERS];
static double _start_time[MAX_TIMERS];

void init_rnd(unsigned int rseed) {
    _rnd_seed = rseed;
}

int get_int_rnd(int range) {
    if (range <= 0) {
        FAIL("Range for get_int_rnd <= 0: %d\n", range);
    }
    int rnd_index = ((double)range * rand_r(&_rnd_seed)) / RAND_MAX;
    if (rnd_index > range - 1 || rnd_index < 0) {
        FAIL("Error in random number generation index out of range, %d > %d\n", 
             rnd_index, range - 1);
    }
    return rnd_index;
}

double get_double_rnd(double min, double max) {
    return ((double)rand_r(&_rnd_seed)) / (double)RAND_MAX * (max - min) + min;
}

double _get_current_time(void) {
    struct timespec ts;
    syscall(__NR_clock_gettime, CLOCK_REALTIME, &ts);
    double t = (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
    return t;
}

void clear_timers() {
    for (int i = 0; i < MAX_TIMERS; i++) timer_clear(i);
}

void timer_clear(int n) {
    _elapsed_time[n] = 0.0;
}

void timer_start(int n) {
    _start_time[n] = _get_current_time();
}

void timer_stop(int n) {
    double t, now;
    now = _get_current_time();
    t = now - _start_time[n];
    _elapsed_time[n] += t;
}

double timer_read(int n) {
    return(_elapsed_time[n]);
}

// We use this instead of strstr because that generates warnings in UPC
int in_str(const char* str, const char* substr) {
    int sub_i = 0;
    int started = 0;
    for (int i = 0; i < strlen(str); i++) {
        if (!started) {
            if (str[i] == substr[sub_i] ) started = 1;
        }
        if (started) {
            if (str[i] != substr[sub_i]) return 0;
            sub_i++;
            if (sub_i == strlen(substr)) return 1;
        }
    }
    return 0;

}

void mfprintf(FILE* f, int num_args, ...) {
    va_list ap;
    va_start(ap, num_args);
    for (int i = 0; i < num_args; i++) {
        char* fmt = va_arg(ap, char*);
        // now determine what type the arg should be
        if (in_str(fmt, "d")) fprintf(f, fmt, va_arg(ap, int));
        if (in_str(fmt, "s")) fprintf(f, fmt, va_arg(ap, char*));
        if (in_str(fmt, "f")) fprintf(f, fmt, va_arg(ap, double));
        if (in_str(fmt, "e")) fprintf(f, fmt, va_arg(ap, double));
    }
    va_end(ap);
}

