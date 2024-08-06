#include <common_cpp.h>

#include <functional>
#include <memory>
#include <filesystem>

#include <myheap.h>

void parse_args(
    int argc, char** argv,
    const char* shortopts,
    void(*args_handler)(
        const int opt,
        const int optopt,
        const int optind,
        char* optarg
    )
)
{
    extern char* optarg;
    extern int   optopt;
    extern int   optind;

    int opt;
    while ((opt = getopt(argc, argv, shortopts)) != -1)
    {
        args_handler(opt, optopt, optind, optarg);
    }
}


void kmerge_file(
    std::vector<std::string> input_file_list,
    std::string output_file_path
)
{
    std::function<
        bool(
            const std::pair<std::shared_ptr<std::ifstream>, int>&,
            const std::pair<std::shared_ptr<std::ifstream>, int>&
        )
    > cmp = [](
        const std::pair<std::shared_ptr<std::ifstream>, int>& fp1,
        const std::pair<std::shared_ptr<std::ifstream>, int>& fp2
    ) {
        return fp1.second > fp2.second; // ascend heap, not descend heap
    };
    heap<std::pair<std::shared_ptr<std::ifstream>, int>, decltype(cmp)> ksegheap(cmp);

    for (const std::string input_file_path : input_file_list)
    {
        std::shared_ptr<std::ifstream> finput = std::make_shared<std::ifstream>(
            input_file_path, std::ifstream::in | std::ifstream::binary
        );
        if (!finput->is_open()) {
            fprintf(stderr, "failed to open %s\n", input_file_path.c_str());
            continue;
        }

        int finput_head;
        finput->read(reinterpret_cast<char*>(&finput_head), 1 * sizeof(int));
        if (finput->gcount() == 0)
            continue;
        // can't use std::pair<type1, type2> to construct
        // there is no such constructor for std::pair
        ksegheap.push(std::make_pair(finput, finput_head));
    }

    std::ofstream foutput(output_file_path, std::ofstream::out | std::ofstream::binary);
    std::filesystem::create_directories(std::filesystem::path(output_file_path).parent_path());
    if (!foutput.is_open())
    {
        fprintf(stderr, "failed to open kmerge file output file %s\n", output_file_path.c_str());
        exit(4);
    }

    while (!ksegheap.empty())
    {
        auto [finput, finput_head] = ksegheap.top();
        ksegheap.pop();
        foutput.write(reinterpret_cast<char*>(&finput_head), 1 * sizeof(int));

        finput->read(reinterpret_cast<char*>(&finput_head), 1 * sizeof(int));
        if (finput->gcount() == 0)
            finput->close();
        else
            ksegheap.push(std::make_pair(finput, finput_head));
    }
    foutput.close();
}

/*
* internal_buf_size - vector size for external sort
* proc_mark - used for MPI environment
* keep_size - number of items will actually be saved to file, -1 for all
* sort_order - 0: ascend, 1: descend
* save_order - 0: ascend, 1: descend
*/
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
    std::vector<int> rx_buf(internal_buf_size);
    char file_path[128];

    do {
        finput.read(reinterpret_cast<char*>(rx_buf.data()), sizeof(int) * internal_buf_size);
        rx_cnt = finput.gcount() / sizeof(int);
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
) {
    if (buffsz <= 0 || typesz <= 0)
    {
        fprintf(stderr, "invalid arguments buffsz=%d, typesz=%d\n", buffsz, typesz);
        exit(-1);
    }

    FILE* rx_ptr = fopen(file_path, "r+");
    if (rx_ptr == NULL)
    {
        fprintf(stderr, "failed to open file %s\n", file_path);
        exit(1);
    }

    FILE* tx_ptr = fopen(file_path, "r+");
    if (tx_ptr == NULL)
    {
        fprintf(stderr, "failed to open file %s\n", file_path);
        exit(1);
    }

    char* buf = (char*)malloc(typesz * buffsz);
    if (buf == NULL)
    {
        fprintf(stderr, "failed to allocate buffer with size %d\n", buffsz);
        exit(3);
    }

    if (fseek(rx_ptr, offset * typesz, SEEK_SET)) exit(1);
    if (fseek(tx_ptr, 0, SEEK_SET)) exit(1);

    int movect = 0;
    int rx_cnt = 0;
    do {
        rx_cnt = fread(buf, typesz, min(buffsz, movesz - movect), rx_ptr);
        // printf("%s\n", buf);
        fwrite(buf, typesz, rx_cnt, tx_ptr);

        movect += rx_cnt;
    } while (movect < movesz && rx_cnt == buffsz);


    fseek(tx_ptr, 0, SEEK_SET);
    if (ftruncate(fileno(tx_ptr), movect * typesz) != 0)
    {
        fprintf(stderr, "failed to truncate result file\n");
        exit(2);
    }

    fclose(rx_ptr);
    fclose(tx_ptr);
    free(buf);
}
