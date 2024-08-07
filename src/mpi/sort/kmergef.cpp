#include <common_cpp.h>

#include <getopt.h>
#include <ctype.h>

#include <functional>
#include <myheap.h>
#include <memory>

// global data and option
int buf_sze = 0;
char* file_path = nullptr;
bool delete_temp = false;

// global function
void args_handler(
    const int opt,
    const int optopt,
    const int optind,
    char* optarg
) {
    switch (opt)
    {
    case 'f':
        file_path = optarg;
        break;
    
    case 'D':
        delete_temp = true;
        break;
    
    case 'b':
        if ((buf_sze = atoi(optarg)) <= 0)
        {
            // atoi can not tell if a conversion is failed
            fprintf(stderr, "invalid buffer size %s\n", optarg);
            exit(1);
        }
        break;
    case '?':
        if (isprint(optopt))
            fprintf(stderr, "Unknown option `-%c'.\n", optopt);
        else
            fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
        break;
    default:
        abort();
    }
}

int main(int argc, char** argv)
{
    using std::cout;
    using std::cerr;
    using std::endl;

    parse_args(argc, argv, "f:Db:", &args_handler);
    cout << file_path << buf_sze << endl;
    if (buf_sze == 0 || file_path == nullptr)
    {
        cout << "usage: kmergef -b <buffer size> -f <bin data path> [-D]" << endl;
        exit(1);
    }

    srand((unsigned int)time(NULL));
    
    int seg_cnt = 0; // the number of segments
    {
        // temporary read area
        int* tmp_buf = new int[buf_sze];

        std::ifstream finput(file_path, std::ifstream::in | std::ifstream::binary);
        if (!finput.is_open())
        {
            std::cerr << "failed to open data bin " << file_path << std::endl;
            exit(1);
        }
        int rx_cnt = 0;
        do {
            finput.read(reinterpret_cast<char*>(tmp_buf), buf_sze * sizeof(int));
            rx_cnt = finput.gcount();
            if (rx_cnt == 0) break; // read to end

            // use quick sort on plain array
            std::sort(tmp_buf, tmp_buf + rx_cnt / sizeof(int));
            
            // split to sub file
            char output_file_path[128];
            sprintf(output_file_path, "data/temp/%d.bin", seg_cnt);
            std::ofstream foutput(output_file_path, std::ofstream::out | std::ofstream::binary);
            if (!foutput.is_open())
            {
                std::cerr << "failed to open data bin " << output_file_path << std::endl;
                exit(1);
            }
            foutput.write(reinterpret_cast<char*>(tmp_buf), rx_cnt);
            foutput.close();
            seg_cnt++;
            printf("finish segment#%d with %ld element(s)\n", seg_cnt, rx_cnt / sizeof(int));
        } while (rx_cnt == buf_sze * sizeof(int));
        finput.close();

        delete[] tmp_buf;
    }
    std::cout << "distributed to " << seg_cnt << " segment(s)" << std::endl;


    /*
    ============================== division ==============================
    ============================== division ==============================
    ============================== division ==============================
    ============================== division ==============================
    ============================== division ==============================
    */
    // start k-way heap merge
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
    for (int i = 0; i < seg_cnt; ++i)
    {
        char output_file_path[128];
        sprintf(output_file_path, "data/temp/%d.bin", i);
        std::shared_ptr<std::ifstream> finput = std::make_shared<std::ifstream>(
            output_file_path, std::ifstream::in | std::ifstream::binary
        );
        if (!finput->is_open()) {
            fprintf(stderr, "failed to open %s\n", output_file_path);
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
    
    std::ofstream foutput("data/sorted.bin", std::ios::out | std::ios::binary);
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

    /*
    ============================== division ==============================
    ============================== division ==============================
    ============================== division ==============================
    ============================== division ==============================
    ============================== division ==============================
    */
    // clean up staff
    if (delete_temp)
    {
        for (int i = 0; i < seg_cnt; ++i)
        {
            char output_file_path[128];
            sprintf(output_file_path, "data/temp/%d.bin", i);
            if (std::remove(output_file_path) != 0)
            {
                std::perror("failed to delete temp file");
                exit(1);
            }
        }
    }

    return 0;
}

