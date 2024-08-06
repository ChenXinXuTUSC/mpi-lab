#ifndef COMMON_CPP_H
#define COMMON_CPP_H


#include <cstdio>
#include <cstdlib>

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <filesystem>


#include <string>
#include <vector>
#include <queue>

#include <algorithm>
#include <random>
#include <ctime>

// c part
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <ctype.h>


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

void kmerge_file(
    std::vector<std::string> input_file_list,
    std::string output_file_path
);

void sort_file(
    std::string input_file_path,
    std::string output_file_path,
    const int& internal_buf_size,
    const int& proc_mark
);

void c_truncate(
    const char* file_path,
    const int typesz,
    const int offset,
    const int movesz,
    const int buffsz
);


#endif
