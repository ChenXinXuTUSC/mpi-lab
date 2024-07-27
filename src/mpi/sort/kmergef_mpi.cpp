#include <common_cpp.h>
#include <getopt.h>
#include <ctype.h>

#include <mpi/mpi.h>

#include <functional>
#include <memory>
#include <myheap.h>

#include <filesystem>

// global data and option
int buf_sze = 0;
char* bin_data_path = nullptr;
bool delete_temp = false;

// global function
void parse_args(int argc, char** argv);

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
            finput.open(bin_data_path, std::ifstream::in | std::ifstream::binary);
            if (!finput.is_open())
            {
                fprintf(stderr, "failed to open %s\n", bin_data_path);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }

        char file_path[128];
        sprintf(file_path, "data/mpi/node%d/recv.bin", world_rank);
        std::filesystem::create_directories(std::filesystem::path(file_path).parent_path()); // return false if dir already exists
        std::ofstream foutput(file_path, std::ofstream::out | std::ofstream::binary);
        if (!foutput.is_open())
        {
            fprintf(stderr, "failed to open %s\n", file_path);
            MPI_Abort(MPI_COMM_WORLD, 2);
        }

        std::vector<int> send_data(buf_sze);
        std::vector<int> recv_data(buf_sze);
        int rx_cnt;
        int tx_cnt;
        do {
            if (world_rank == 0)
            {
                finput.read(reinterpret_cast<char*>(send_data.data()), sizeof(int) * buf_sze);
                rx_cnt = finput.gcount() / sizeof(int);
            }
            // every node must know the amount to receive
            MPI_Bcast(&rx_cnt, 1, MPI_INT, 0, MPI_COMM_WORLD);
            if (rx_cnt == 0) break; // no data
            tx_cnt = (rx_cnt - 1) / world_size + 1;
            
            MPI_Scatter(
                send_data.data(),
                tx_cnt,
                MPI_INT,
                recv_data.data(),
                tx_cnt,
                MPI_INT,
                0,
                MPI_COMM_WORLD
            );

            // each node dump the receive data to disk
            foutput.write(reinterpret_cast<char*>(recv_data.data()), sizeof(int) * tx_cnt);
        } while (rx_cnt == buf_sze);

        if (world_rank == 0)
            finput.close();
        foutput.close();
    }





    MPI_Barrier(MPI_COMM_WORLD); // end of data distribution





    // each node sorts its segment
    int seg_cnt = 0;
    {
        int rx_cnt = 0;
        std::vector<int> rx_buf(buf_sze);
        char file_path[128];

        sprintf(file_path, "data/mpi/node%d/recv.bin", world_rank);
        std::ifstream finput(file_path, std::ifstream::in | std::ifstream::binary);
        if (!finput.is_open())
        {
            // exit this process only
            fprintf(stderr, "node%d failed to open segment %s, exit...\n", world_rank, file_path);
            exit(1);
        }
        do {
            finput.read(reinterpret_cast<char*>(rx_buf.data()), sizeof(int) * buf_sze);
            rx_cnt = finput.gcount() / sizeof(int);
            if (rx_cnt == 0) break;

            std::sort(rx_buf.begin(), rx_buf.end());

            sprintf(file_path, "data/mpi/node%d/%d.bin", world_rank, seg_cnt);
            std::ofstream foutput(file_path, std::ofstream::out | std::ofstream::binary);
            std::filesystem::create_directories(std::filesystem::path(file_path).parent_path());
            if (!foutput.is_open())
            {
                fprintf(stderr, "node %d failed to dump sorted sub-segment %d", world_rank, seg_cnt);
                continue;
            }

            foutput.write(reinterpret_cast<char*>(rx_buf.data()), sizeof(int) * rx_cnt);
            foutput.close();

            seg_cnt++;
        } while (rx_cnt == buf_sze);
    }





    /*
    ============================== division ==============================
    ============================== division ==============================
    ============================== division ==============================
    ============================== division ==============================
    ============================== division ==============================
    */
    // start k-way heap merge
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

        char file_path[128];
        for (int i = 0; i < seg_cnt; ++i)
        {
            sprintf(file_path, "data/mpi/node%d/%d.bin", world_rank, i);
            std::shared_ptr<std::ifstream> finput = std::make_shared<std::ifstream>(
                file_path, std::ifstream::in | std::ifstream::binary
            );
            if (!finput->is_open()) {
                fprintf(stderr, "failed to open %s\n", file_path);
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

        sprintf(file_path, "data/mpi/node%d/sorted.bin", world_rank);
        std::ofstream foutput(file_path, std::ios::out | std::ios::binary);
        std::filesystem::create_directories(std::filesystem::path(file_path).parent_path());
        if (!foutput.is_open())
        {
            fprintf(stderr, "node%d failed to open sorted segment\n", world_rank);
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



    MPI_Barrier(MPI_COMM_WORLD); // end of each segment sort



    if (world_rank == 0)
    {
        // do the external k merge again
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

        char file_path[128];

        for (int i = 0; i < world_size; ++i)
        {
            sprintf(file_path, "data/mpi/node%d/sorted.bin", i);
            std::shared_ptr<std::ifstream> finput = std::make_shared<std::ifstream>(
                file_path, std::ifstream::in | std::ifstream::binary
            );
            if (!finput->is_open()) {
                fprintf(stderr, "failed to open %s\n", file_path);
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

        std::ofstream foutput("data/mpi/sorted.bin", std::ios::out | std::ios::binary);
        std::filesystem::create_directories(std::filesystem::path(file_path).parent_path());
        if (!foutput.is_open())
        {
            fprintf(stderr, "node%d failed to open sorted segment\n", world_rank);
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


    MPI_Barrier(MPI_COMM_WORLD); // end of each segment merge

    if (delete_temp)
    {
        char output_file_path[128];
        sprintf(output_file_path, "data/mpi/node%d/recv.bin", world_rank);
        if (std::remove(output_file_path) != 0)
        {
            std::perror("failed to delete temp file");
            MPI_Abort(MPI_COMM_WORLD, 3);
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
            if ((buf_sze = atoi(optarg)) <= 0)
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
