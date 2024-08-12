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

template<typename dtype>
void sort_file(
    std::string input_file_path,
    std::string output_file_path,
    const int& internal_buf_size,
    const int& proc_mark
);

/*
* internal_buf_size - vector size for external sort
* proc_mark - used for MPI environment
* keep_size - number of items will actually be saved to file, -1 for all
* sort_order - 0: ascend, 1: descend
* save_order - 0: ascend, 1: descend
*/
template<typename dtype>
void sort_file(
    std::string input_file_path,
    std::string output_file_path,
    const int& internal_buf_size,
    const int& proc_mark
)
{
    // some data variables

    std::ifstream finput(input_file_path, std::ifstream::in | std::ifstream::binary);
    if (!finput.is_open())
    {
        // exit this process only
        fprintf(stderr, "node%d sort_file failed to open data bin %s, exit...\n", proc_mark, input_file_path.c_str());
        exit(1);
    }

    int seg_cnt = 0;
    int rx_cnt;
    std::vector<dtype> rx_buf(internal_buf_size);
    char file_path[128];

    do {
        finput.read(reinterpret_cast<char*>(rx_buf.data()), sizeof(dtype) * internal_buf_size);
        rx_cnt = finput.gcount() / sizeof(dtype);
        if (rx_cnt == 0) break;

        std::sort(rx_buf.begin(), rx_buf.end());

        sprintf(file_path, "data/temp/node%d/seg%d.bin", proc_mark, seg_cnt);
        std::filesystem::create_directories(std::filesystem::path(file_path).parent_path());
        std::ofstream foutput(file_path, std::ofstream::out | std::ofstream::binary);
        if (!foutput.is_open())
        {
            fprintf(stderr, "node%d failed to dump sorted sub-segment %d", proc_mark, seg_cnt);
            continue;
        }

        foutput.write(reinterpret_cast<char*>(rx_buf.data()), sizeof(int) * rx_cnt);
        foutput.close();

        seg_cnt++;
    } while (rx_cnt == internal_buf_size);
    finput.close();


    std::vector<std::string> input_file_list;
    for (int i = 0; i < seg_cnt; ++i)
    {
        sprintf(file_path, "data/temp/node%d/seg%d.bin", proc_mark, i);
        input_file_list.emplace_back(std::string(file_path));
    }
    kmerge_file(input_file_list, output_file_path);
}

void c_truncate(
    const char* file_path,
    const int typesz,
    const int offset,
    const int movesz,
    const int buffsz
);


#endif
