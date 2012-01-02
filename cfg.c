#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cfg.h"

static struct option lopts[] = {
    {"rseed", 1, 0, 'd'},
    {"num_iters", 1, 0, 'i'},
    {"num_ags", 1, 0, 'n'},
    {"av_max_csmp", 1, 0, 'c'},
    {"av_max_prod", 1, 0, 'p'},
    {"prdr_sample_size", 1, 0, 'z'},
    {"verbose_flags", 1, 0, 'v'},
    {"help", 0, 0, 'h'},
    {0, 0, 0, 0}
};


const char* opts_help[] = {
	"random seed",
    "number of iterations",
    "number of agents",
    "max. consumption",
    "max. production",
    "sample size for getting cheapest producer",
    "verbose-mode flags",
    "this help"};

void load_cfg(int argc, char** argv, cfg_t* cfg) {
    char verbose_flag_help[5000] = "verbose: ";
    char buf[5000];
    int vflags_len = sizeof(VERBOSE_FLAGS) / sizeof(verbose_flag_t);
    for (int i = 0; i < vflags_len; i++) {
        sprintf(buf, "%c=%s,", VERBOSE_FLAGS[i].flag, VERBOSE_FLAGS[i].name);
        strcat(verbose_flag_help, buf);
        if (i == 4) strcat(verbose_flag_help, "\n                              ");
    }
    // set defaults
    // these are set to give reasonable outputs
	cfg->rseed = 31;
    cfg->num_iters = 100000;
    cfg->num_ags = 100;
    cfg->av_max_csmp = 10.0;
    cfg->av_max_prod = 10.0;
    cfg->prdr_sample_size = 10;
    cfg->verbose_flags = VFLAG_STATS;
    char verbose_str[5000] = "";
    // get cfgs
    char optstr[100] = "+";
    for (int i = 0, j = 1; lopts[i].name; i++) {
        optstr[j++] = lopts[i].val;
        if (lopts[i].has_arg) optstr[j++] = ':';
    }
    int opt;
    while (1) {
        int option_index = 0;
        opt = getopt_long(argc, argv, optstr, lopts, &option_index);
        if (opt == -1) break;
        switch (opt) {
        case 'd': cfg->rseed = atoi(optarg); break;
        case 'i': cfg->num_iters = atoi(optarg); break;
        case 'n': cfg->num_ags = atoi(optarg); break;
        case 'c': cfg->av_max_csmp = atof(optarg); break;
        case 'p': cfg->av_max_prod = atof(optarg); break;
        case 'z': cfg->prdr_sample_size = atoi(optarg); break;
        case 'v': 
            for (int i = 0; i < strlen(optarg); i++) {
                for (int v = 0; v < vflags_len; v++) {
                    if (optarg[i] == VERBOSE_FLAGS[v].flag) {
                        cfg->verbose_flags |= VERBOSE_FLAGS[v].index;
                        sprintf(buf, "%c", VERBOSE_FLAGS[v].flag);
                        strcat(verbose_str, buf);
                    }
                }
            }
            break;
        default: opt = 'h';
        }
        if (opt == 'h') break;
    }
    if (opt == 'h') {
        // print help
        printf("Usage:  %s options\nWhere options are:\n", argv[0]);
        int i = 0;
        while (lopts[i].name) {
            if (!strcmp(lopts[i].name, "verbose_flags")) {
                printf("   -%c --%-20s: %s\n", lopts[i].val, lopts[i].name, 
                       verbose_flag_help);
            } else {
                printf("   -%c --%-20s: %s\n", 
                       lopts[i].val, lopts[i].name, opts_help[i]);
            }
            i++;
        }
        exit(EXIT_FAILURE);
    }
    print_cfg(cfg, ' ', stdout);
    fprintf(stdout, "%c  -v%s\n", ' ', verbose_str);
}

#define PRINT_INT_OPT(opt)                                              \
    fprintf(f, "%c  -%c %-50s %8d\n", comment, lopts[i].val, opts_help[i], opt); \
    i++; 

#define PRINT_DOUBLE_OPT(opt)                                              \
    fprintf(f, "%c  -%c %-50s %8.2f\n", comment, lopts[i].val, opts_help[i], opt); \
    i++; 

void print_cfg(cfg_t* cfg, char comment, FILE* f) {
    int i = 0;
	PRINT_INT_OPT(cfg->rseed);
    PRINT_INT_OPT(cfg->num_iters);
    PRINT_INT_OPT(cfg->num_ags);
    PRINT_DOUBLE_OPT(cfg->av_max_csmp);
    PRINT_DOUBLE_OPT(cfg->av_max_prod);
    PRINT_INT_OPT(cfg->prdr_sample_size);
}



