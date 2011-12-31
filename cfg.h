#ifndef _CFG_H
#define _CFG_H


#include <stdio.h>

typedef struct {
    int index;
    char flag;
    char* name;
} verbose_flag_t;

#define VFLAG_TIMERS 1
#define VFLAG_AGENTS 2
#define VFLAG_PC_LISTS 4
#define VFLAG_CONSUME 8
#define VFLAG_CONSUME_DETAILS 16
#define VFLAG_STATS 32

static const verbose_flag_t VERBOSE_FLAGS[] = {
    {.index = VFLAG_TIMERS, .flag = 'T', .name = "timers"},
    {.index = VFLAG_AGENTS, .flag = 'A', .name = "agents"},
    {.index = VFLAG_PC_LISTS, .flag = 'L', .name = "prdr csmr lists"},
    {.index = VFLAG_CONSUME, .flag = 'C', .name = "consume"},
    {.index = VFLAG_CONSUME_DETAILS, .flag = 'D', .name = "consume details"},
    {.index = VFLAG_STATS, .flag = 'S', .name = "show stats every iter"},
};

#define DBG(FLAG, fmt, ...)                                             \
    do {                                                                \
        if (FLAG & _cfg.verbose_flags)                                  \
            printf("[%d] " fmt, _iters, __VA_ARGS__);                   \
    } while (0);

#define DBG_START(FLAG) if (_cfg.verbose_flags & FLAG) 

typedef struct {
	int rseed;
    int num_iters;
    int num_ags;
    double av_money_start;
    double av_svgs_level;
    int min_csmp;
    int av_max_csmp;
    int av_max_prod;
    int av_exptd_prod;
    double min_prod_price;
    double price_adjust;
    int prdr_sample_size;
    int verbose_flags;    
} cfg_t;

void load_cfg(int argc, char** argv, cfg_t* cfg);
void print_cfg(cfg_t* cfg, char comment, FILE* f);

#endif

