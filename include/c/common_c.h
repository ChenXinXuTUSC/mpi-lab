#ifndef COMMON_C_H
#define COMMON_C_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>

#include <getopt.h>
#include <ctype.h>

#include <math.h>

// function

void parse_args(
    int argc, char** argv,
    const char* shortopts,
    void(*args_handler)(
        const int opt,
        const int optopt,
        const int optind,
        char* optarg
    )
);

#endif
