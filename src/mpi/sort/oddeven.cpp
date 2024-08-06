#include <common_cpp.h>

#include <mpi/mpi.h>

#include <getopt.h>
#include <ctype.h>

// global data and option
int buf_size = 0;
char* bin_data_path = nullptr;
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

int main(int argc, char** argv)
{
    parse_args(argc, argv, "Df:b:", &args_handler);

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

        int rx_cnt = 0;
        int tx_cnt = 0;
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
            // printf("[phase %d node%d] %d<->%d\n", phase, world_rank, world_rank, partner_rank);

            // preparation for MPI_Sendrecv
            // input file
            sprintf(file_path, "data/mpi/node%d/sorted.bin", world_rank);
            std::ifstream finput(file_path, std::ifstream::binary);
            if (!finput.is_open())
            {
                fprintf(stderr, "node%d failed to open %s during phase %d\n", world_rank, file_path, phase);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            // output file
            sprintf(file_path, "data/mpi/node%d/sorted_partner.bin", world_rank);
            std::filesystem::create_directories(std::filesystem::path(std::string(file_path)).parent_path());
            std::ofstream foutput(file_path, std::ofstream::binary);
            if (!foutput.is_open())
            {
                fprintf(stderr, "node%d failed to open %s during phase %d\n", world_rank, file_path, phase);
                MPI_Abort(MPI_COMM_WORLD, 2);
            }


            // record the send and recv data amount
            int tx_ttl = 0;
            int rx_ttl = 0;
            // start data exchange
            MPI_Status status;
            do {
                if (finput.is_open())
                {
                    finput.read(reinterpret_cast<char*>(tx_buf.data()), sizeof(int) * buf_size);
                    tx_cnt = finput.gcount() / sizeof(int);
                    tx_ttl += tx_cnt;
                    if (tx_cnt == 0) finput.close();
                }
                MPI_Sendrecv(
                    tx_buf.data(), tx_cnt,   MPI_INT, partner_rank, 0, // send to partner
                    rx_buf.data(), buf_size, MPI_INT, partner_rank, 0, // receive from partner
                    MPI_COMM_WORLD, &status
                );
                MPI_Get_count(&status, MPI_INT, &rx_cnt);
                rx_ttl += rx_cnt;
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
            std::filesystem::rename(merge_file_path, input_file_path1); // replace the original "sorted.bin"

            // truncate corresponding part of each node
            {
                // tx_ttl represents node's segment size
                // rx_ttl represents partner's segment size
                if (world_rank < partner_rank)
                {
                    // keep the smaller part
                    c_truncate(input_file_path1.c_str(), sizeof(int), 0     , tx_ttl, buf_size);
                }
                else
                {
                    // keep the larger part
                    c_truncate(input_file_path1.c_str(), sizeof(int), rx_ttl, tx_ttl, buf_size);
                }
            }

            // printf(
            //     "[node%d phase%d (%d<->%d)]: rx %d, tx %d, truncate %ld\n",
            //     world_rank, phase, world_rank, partner_rank,
            //     rx_ttl, tx_ttl, std::filesystem::file_size(input_file_path1) / sizeof(int)
            // );
        }

    }
    MPI_Barrier(MPI_COMM_WORLD);


    if (world_rank == 0)
    {
        char file_path[128];
        std::vector<int> in_buf(buf_size);

        std::filesystem::create_directories("data/output");
        std::ofstream foutput("data/output/final.bin", std::ofstream::binary);
        // only need one process to merge all sorted segments
        for (int i = 0; i < world_size; ++i)
        {
            sprintf(file_path, "data/mpi/node%d/sorted.bin", i);
            std::ifstream finput(file_path, std::ifstream::binary);
            if (!finput.is_open())
            {
                fprintf(stderr, "node0 failed to open reduce seg %s\n", file_path);
                exit(1);
            }

            int in_cnt = 0;
            int in_ttl = 0;
            do {
                finput.read(reinterpret_cast<char*>(in_buf.data()), sizeof(int) * buf_size);
                in_cnt = finput.gcount() / sizeof(int);
                in_ttl += in_cnt;
                foutput.write(reinterpret_cast<char*>(in_buf.data()), sizeof(int) * in_cnt);
            } while (in_cnt == buf_size);
            // printf("%s %d\n", file_path, in_ttl);
            finput.close();
        }
        foutput.close();
    }

    if (delete_temp && world_rank == 0)
    {
        try
        {
            if (std::filesystem::exists("data/mpi"))
                std::filesystem::remove_all("data/mpi");
            if (std::filesystem::exists("data/temp"))
                std::filesystem::remove_all("data/temp");
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            std::cerr << "remove temporary files error: " << e.what() << std::endl;
        }
    }

    MPI_Finalize();
    return 0;
}
