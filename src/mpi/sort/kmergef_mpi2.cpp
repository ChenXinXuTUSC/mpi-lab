#include <common_cpp.h>
#include <getopt.h>
#include <ctype.h>

#include <mpi/mpi.h>

#include <functional>
#include <memory>
#include <myheap.h>

#include <filesystem>

// global data and option
int buf_size = 0;
char* bin_data_path = nullptr;
bool delete_temp = false;

// global function
void parse_args(int argc, char** argv);

void kmerge_file(std::vector<std::string> input_list, std::string output_list);

void sort_file(std::string input_file_path, std::string output_file_path, const int& internal_buf_size, const int& proc_mark);

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

        std::vector<int> send_data(buf_size);
        std::vector<int> recv_data(buf_size);
        int rx_cnt;
        int tx_cnt;
        do {
            if (world_rank == 0)
            {
                finput.read(reinterpret_cast<char*>(send_data.data()), sizeof(int) * buf_size);
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


    // logn merge by multiple process nodes
    {
        std::vector<int> send_data(buf_size);
        std::vector<int> recv_data(buf_size);
        int rx_cnt;
        int tx_cnt;

        for (int i = 2; i <= world_size; i *= 2)
        {
            // if (world_rank == 0)
            //     printf("=========================== stride %d ===========================\n", i);
            if (world_rank % i == 0)
            {
                // this node should receive from partner
                // and do the merge
                int partner_rank = world_rank + i / 2;
                // printf("node%d will receive from node%d\n", world_rank, partner_rank);
                char file_path[128];
                sprintf(file_path, "data/mpi/node%d/sorted_partner.bin", world_rank);
                std::filesystem::create_directories(std::filesystem::path(file_path).parent_path());
                std::ofstream foutput(file_path, std::ofstream::out | std::ofstream::binary);
                if (!foutput.is_open())
                {
                    fprintf(stderr, "node%d failed to receive partner node%d's sorted data\n", world_rank, partner_rank);
                    MPI_Abort(MPI_COMM_WORLD, 5);
                }

                MPI_Status recv_status;
                do {
                    MPI_Recv(recv_data.data(), buf_size, MPI_INT, partner_rank, 0, MPI_COMM_WORLD, &recv_status);
                    MPI_Get_count(&recv_status, MPI_INT, &rx_cnt);

                    foutput.write(reinterpret_cast<char*>(recv_data.data()), sizeof(int) * rx_cnt);
                } while (rx_cnt == buf_size);
                foutput.close();

                // merge into one sorted segment
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
                // prepare for next merge read
                std::filesystem::rename(merge_file_path, input_file_path1);
            }
            else if ((world_rank - i / 2) >= 0 && (world_rank - i / 2) % i == 0)
            {
                // this node should send to partner
                int partner_rank = world_rank - i / 2;
                // printf("node%d will send to node%d\n", world_rank, partner_rank);
                char file_path[128];
                sprintf(file_path, "data/mpi/node%d/sorted.bin", world_rank);
                std::ifstream finput(file_path, std::ifstream::in | std::ifstream::binary);
                if (!finput.is_open())
                {
                    fprintf(stderr, "node%d failed to open sorted data bin file\n", world_rank);
                    MPI_Abort(MPI_COMM_WORLD, 5);
                }

                do {
                    finput.read(reinterpret_cast<char*>(send_data.data()), sizeof(int) * buf_size);
                    tx_cnt = finput.gcount() / sizeof(int);

                    MPI_Send(send_data.data(), tx_cnt, MPI_INT, partner_rank, 0, MPI_COMM_WORLD);
                } while (tx_cnt == buf_size);
                finput.close();
            }
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);

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

void kmerge_file(std::vector<std::string> input_file_list, std::string output_file_path)
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

void sort_file(std::string input_file_path, std::string output_file_path, const int& internal_buf_size, const int& proc_mark)
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
