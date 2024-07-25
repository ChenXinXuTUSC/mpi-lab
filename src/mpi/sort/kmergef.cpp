/*
 * arguments:
 * -f file path to the data bin
 * -D delete temp file after merge sort
 * -b buffer size of each segment
*/
#include <common_cpp.h>

#include <getopt.h>
#include <ctype.h>

// global data and option
int buf_sze;
char* file_path;
bool delete_temp;

// global function
void parse_args(int argc, char** argv);

int main(int argc, char** argv)
{
    parse_args(argc, argv);

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
            
            // split to sub file
            char output_file_path[128];
            sprintf(output_file_path, "data/temp/%d.bin", seg_cnt);
            std::ofstream foutput(output_file_path, std::ostream::out | std::ostream::binary);
            if (!foutput.is_open())
            {
                std::cerr << "failed to open data bin " << output_file_path << std::endl;
                exit(1);
            }
            foutput.write(reinterpret_cast<char*>(tmp_buf), buf_sze * sizeof(int));

            
            seg_cnt++;
        } while (rx_cnt == buf_sze * sizeof(int));
        finput.close();

        delete[] tmp_buf;
    }

    std::cout << "distributed to " << seg_cnt << " segment(s)" << std::endl;
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

void parse_args(int argc, char** argv)
{
    extern char* optarg;
    extern int   optopt;
    extern int   optind;

    int opt;
    while ((opt = getopt(argc, argv, "f:Db:")) != -1)
    {
        switch (opt)
        {
        case 'f':
            file_path = optarg;
            break;
        
        case 'D':
            delete_temp = true;
            break;
        
        case 'b':
            if (buf_sze = atoi(optarg) < 0)
            {
                // atoi can not tell if a conversion is failed
                fprintf(stderr, "invalid buffer size %d\n", buf_sze);
                exit(1);
            }
            break;
        case '?':
            if (optopt == 'o')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            break;
        default:
            abort();
        }
    }
}

