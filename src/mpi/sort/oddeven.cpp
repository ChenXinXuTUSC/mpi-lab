#include <common_cpp.h>

#include <mpi/mpi.h>

#include <getopt.h>
#include <ctype.h>

#include <functional>
#include <memory>
#include <filesystem>

#include <myheap.h>

// global data and option
int buf_size = 0;
char* bin_data_path = nullptr;
bool delete_temp = false;

// global function
void parse_args(int argc, char** argv);

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

int main(int argc, char** argv)
{
    parse_args(argc, argv);

    srand((unsigned int)time(NULL));

    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // distribute data to all nodes
    {
        std::ifstream finput;
        if (world_rank == 0)
        {
            // master node is respnsible for reading in the data
            finput.open(bin_data_path, std::ifstream::binary);
            if (!finput.is_open())
            {
                fprintf(stderr, "failed to open %s\n", bin_data_path);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }

        char file_path[128];
        sprintf(file_path, "data/mpi/node%d/recv.bin", world_rank);
        std::filesystem::create_directories(std::filesystem::path(file_path).parent_path()); // return false if dir already exists
        std::ofstream foutput(file_path, std::ofstream::binary);
        if (!foutput.is_open())
        {
            fprintf(stderr, "failed to open %s\n", file_path);
            MPI_Abort(MPI_COMM_WORLD, 2);
        }

        std::vector<int> tx_buf(buf_size);
        std::vector<int> rx_buf(buf_size);
        int rx_cnt;
        int tx_cnt;
        do {
            if (world_rank == 0)
            {
                finput.read(reinterpret_cast<char*>(tx_buf.data()), sizeof(int) * buf_size);
                rx_cnt = finput.gcount() / sizeof(int);
            }
            // every node receive signal from main node
            MPI_Bcast(&rx_cnt, 1, MPI_INT, 0, MPI_COMM_WORLD);
            if (rx_cnt == 0) break; // every node exit distribution stage

            tx_cnt = (rx_cnt - 1) / world_size + 1; // divided by each process node, (a-1)/b+1 is ceiling formula
            MPI_Scatter(
                tx_buf.data(),
                tx_cnt,
                MPI_INT,
                rx_buf.data(),
                tx_cnt,
                MPI_INT,
                0,
                MPI_COMM_WORLD
            );

            // each node dump the receive data to disk
            foutput.write(reinterpret_cast<char*>(rx_buf.data()), sizeof(int) * tx_cnt);
        } while (rx_cnt == buf_size);

        if (world_rank == 0)
            finput.close();
        foutput.close();
    }
    MPI_Barrier(MPI_COMM_WORLD); // end of data distribution


    // each proc sort its segment
    {
        char file_path[128];
        sprintf(file_path, "data/mpi/node%d/recv.bin", world_rank);
        std::string input_file_path = std::string(file_path);
        sprintf(file_path, "data/mpi/node%d/sorted.bin", world_rank);
        std::string output_file_path = std::string(file_path);
        sort_file(input_file_path, output_file_path, buf_size, world_rank);
    }
    MPI_Barrier(MPI_COMM_WORLD); // end of data each node file sort




    // segment prepare finish, now start odd even sort algorithm
    {
        int partner_rank;
        char file_path[128];

        int rx_cnt;
        int tx_cnt;
        int file_size_befor_merge; // save for merge result filtration
        std::vector<int> rx_buf(buf_size);
        std::vector<int> tx_buf(buf_size);

        for (int phase = 0; phase < world_size; ++phase)
        {
            if (world_rank % 2 == 0)
            {
                if (phase % 2 == 0) partner_rank = world_rank + 1;
                else                partner_rank = world_rank - 1;
            }
            else
            {
                if (phase % 2 == 0) partner_rank = world_rank - 1;
                else                partner_rank = world_rank + 1;
            }
            if (partner_rank < 0 || partner_rank == world_size) continue; // idle pass, no partner
            printf("[phase %d node%d] %d<->%d\n", phase, world_rank, world_rank, partner_rank);

            // preparation for MPI_Sendrecv
            // input file
            sprintf(file_path, "data/mpi/node%d/sorted.bin", world_rank);
            std::ifstream finput(file_path, std::ifstream::binary);
            if (!finput.is_open())
            {
                fprintf(stderr, "node%d failed to open %s during phase %d\n", world_rank, file_path, phase);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            file_size_befor_merge = std::filesystem::file_size(file_path);
            // output file
            sprintf(file_path, "data/mpi/node%d/sorted_partner.bin", world_rank);
            std::filesystem::create_directories(std::filesystem::path(std::string(file_path)).parent_path());
            std::ofstream foutput(file_path, std::ofstream::binary);
            if (!foutput.is_open())
            {
                fprintf(stderr, "node%d failed to open %s during phase %d\n", world_rank, file_path, phase);
                MPI_Abort(MPI_COMM_WORLD, 2);
            }


            // start data exchange
            MPI_Status status;
            do {
                if (finput.is_open())
                {
                    finput.read(reinterpret_cast<char*>(tx_buf.data()), sizeof(int) * buf_size);
                    tx_cnt = finput.gcount() / sizeof(int);
                    if (tx_cnt == 0) finput.close();
                }
                MPI_Sendrecv(
                    tx_buf.data(), tx_cnt,   MPI_INT, partner_rank, 0, // send to partner
                    rx_buf.data(), buf_size, MPI_INT, partner_rank, 0, // receive from partner
                    MPI_COMM_WORLD, &status
                );
                MPI_Get_count(&status, MPI_INT, &rx_cnt);
                foutput.write(reinterpret_cast<char*>(rx_buf.data()), sizeof(int) * rx_cnt);
            } while (tx_cnt == buf_size || rx_cnt == buf_size);
            foutput.close();



            // start external merge
            std::vector<std::string> input_file_list;
            sprintf(file_path, "data/mpi/node%d/sorted.bin", world_rank);
            std::string input_file_path1 = std::string(file_path);
            input_file_list.emplace_back(input_file_path1);
            sprintf(file_path, "data/mpi/node%d/sorted_partner.bin", world_rank);
            std::string input_file_path2 = std::string(file_path);
            input_file_list.emplace_back(input_file_path2);

            sprintf(file_path, "data/mpi/node%d/merge.bin", world_rank);
            std::string merge_file_path = std::string(file_path);
            kmerge_file(input_file_list, merge_file_path);
            std::filesystem::rename(merge_file_path, input_file_path1); // rename to "sorted.bin"

            // intercept sz_cnt part
            {
                // sprintf(file_path, "data/mpi/node%d/sorted.bin", world_rank);
                // std::ifstream finput(file_path, std::ifstream::binary);
                // if (!finput.is_open())
                // {
                //     fprintf(stderr, "node%d failed to open merge file %s during phase %d\n", world_rank, file_path, phase);
                //     MPI_Abort(MPI_COMM_WORLD, 1);
                // }
                // sprintf(file_path, "data/mpi/node%d/part.bin", world_rank);
                // std::filesystem::create_directories(std::filesystem::path(std::string(file_path)).parent_path());
                // std::ofstream foutput(file_path, std::ofstream::binary);
                // if (!foutput.is_open())
                // {
                //     fprintf(stderr, "node%d failed to open part file %s during phase %d\n", world_rank, file_path, phase);
                //     MPI_Abort(MPI_COMM_WORLD, 2);
                // }

                if (world_rank < partner_rank)
                {
                    // keep the smaller part
                    
                }
                else
                {
                    // keep the bigger part
                }
            }
        }

    }
    MPI_Barrier(MPI_COMM_WORLD);


    if (world_rank == 0)
    {
        // only need one process to merge all sorted segments
        for (int i = 0; i < world_size; ++i)
        {
            
        }
    }

    // put result to output folder
    if (world_rank == 0)
    {
        char file_path[128];
        sprintf(file_path, "data/mpi/node%d/sorted.bin", world_rank);
        std::string result_path = std::string(file_path);
        std::filesystem::create_directories(std::filesystem::path("data/output/final.bin").parent_path());
        std::filesystem::rename(result_path, "data/output/final.bin");
    }

    if (delete_temp)
    {
        if (world_rank == 0)
        {
            try
            {
                if (filesystem::exists("data/mpi"))
                    filesystem::remove_all("data/mpi");
                if (filesystem::exists("data/temp"))
                    filesystem::remove_all("data/temp");
            }
            catch (const filesystem::filesystem_error& e)
            {
                std::cerr << "remove temporary files error: " << e.what() << std::endl;
            }
        }
    }


    MPI_Finalize();
    return 0;
}


void parse_args(int argc, char** argv)
{
    extern char* optarg;
    extern int   optopt;
    extern int   optind;

    int opt;
    while ((opt = getopt(argc, argv, "Df:b:")) != -1)
    {
        switch (opt)
        {
        case 'f':
            bin_data_path = optarg;
            break;
        
        case 'D':
            delete_temp = true;
            break;
        
        case 'b':
            if ((buf_size = atoi(optarg)) <= 0)
            {
                // atoi can not tell if a conversion is failed
                fprintf(stderr, "invalid buffer size %s\n", optarg);
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
        return fp1.second< fp2.second; // ascend heap, not descend heap
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

        sprintf(file_path, "data/temp/node%d/%d.bin", proc_mark, seg_cnt);
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
        sprintf(file_path, "data/temp/node%d/%d.bin", proc_mark, i);
        input_file_list.emplace_back(std::string(file_path));
    }
    kmerge_file(input_file_list, output_file_path);
}
