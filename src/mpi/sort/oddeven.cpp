#include <common_cpp.h>
#include <mpi/mpi.h>

namespace fs = std::filesystem;
using std::endl;
using std::cout;
using std::cerr;
using std::cin;

#ifdef USE_INT
    typedef int dtype;
    #define MPI_DTYPE MPI_INT
#endif

#ifdef USE_FLT
    typedef float dtype;
    #define MPI_DTYPE MPI_FLOAT
#endif


// global data and option
int buf_size = 0;
char* bin_data_path = nullptr;
bool delete_temp = false;

int world_size;
int world_rank;
int master_rank = 0;


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
        if (isprint(optopt))
            fprintf(stderr, "Unknown option `-%c'.\n", optopt);
        else
            fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
        break;
    default:
        abort();
    }
}

void scatter_data(
    const char* input_path,
    const char* output_name,
    const int& source_rank
);

void internal_sort(
    const char* input_name,
    const char* output_name,
    const int& buf_size
);

int main(int argc, char** argv)
{
    parse_args(argc, argv, "Df:b:", &args_handler);

    srand((unsigned int)time(NULL));

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    auto start = std::chrono::high_resolution_clock::now();

    // step1: distribute data to all nodes
    scatter_data(bin_data_path, "recv.bin", master_rank);
    MPI_Barrier(MPI_COMM_WORLD); // end of data distribution


    // step2: each proc sort its segment
    internal_sort("recv.bin", "sorted.bin", buf_size);
    MPI_Barrier(MPI_COMM_WORLD); // end of data each node file sort

    // step3: segment prepare finish, now start odd even sort algorithm
    {
        int partner_rank;

        int rx_cnt = 0;
        int tx_cnt = 0;
        std::vector<dtype> rx_buf(buf_size);
        std::vector<dtype> tx_buf(buf_size);

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

            fs::path base = "data/node";
            // preparation for MPI_Sendrecv
            // input file
            fs::path input_self_path = base / std::to_string(world_rank) / "sorted.bin";
            std::ifstream finput(input_self_path, std::ifstream::binary);
            if (!finput.is_open())
            {
                cerr << "node" << world_rank << " failed to open" << input_self_path << " during phase" << phase << endl;
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            // output file
            fs::path output_partner_path = base / std::to_string(world_rank) / "sorted_partner.bin";
            fs::create_directories(output_partner_path.parent_path());
            std::ofstream foutput(output_partner_path, std::ofstream::binary);
            if (!foutput.is_open())
            {
                cerr << "node" << world_rank << " failed to open" << output_partner_path << " during phase" << phase << endl;
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
                    finput.read(reinterpret_cast<char*>(tx_buf.data()), sizeof(dtype) * buf_size);
                    tx_cnt = finput.gcount() / sizeof(dtype);
                    tx_ttl += tx_cnt;
                    if (tx_cnt == 0) finput.close();
                }
                MPI_Sendrecv(
                    tx_buf.data(), tx_cnt,   MPI_DTYPE, partner_rank, 0, // send to partner
                    rx_buf.data(), buf_size, MPI_DTYPE, partner_rank, 0, // receive from partner
                    MPI_COMM_WORLD, &status
                );
                MPI_Get_count(&status, MPI_INT, &rx_cnt);
                rx_ttl += rx_cnt;
                foutput.write(reinterpret_cast<char*>(rx_buf.data()), sizeof(dtype) * rx_cnt);
            } while (tx_cnt == buf_size || rx_cnt == buf_size);
            foutput.close();

            // start external merge
            std::vector<std::string> input_file_list {
                input_self_path.c_str(),
                output_partner_path.c_str()
            };
            fs::path merge_path = base / std::to_string(world_rank) / "merge.bin";
            kmerge_file<dtype>(input_file_list, merge_path.c_str());
            std::filesystem::rename(merge_path, input_self_path); // replace the original "sorted.bin"

            // truncate corresponding part of each node
            {
                // tx_ttl represents node's segment size
                // rx_ttl represents partner's segment size
                if (world_rank < partner_rank)
                {
                    // keep the smaller part
                    c_truncate(input_self_path.c_str(), sizeof(dtype), 0     , tx_ttl, buf_size);
                }
                else
                {
                    // keep the larger part
                    c_truncate(input_self_path.c_str(), sizeof(dtype), rx_ttl, tx_ttl, buf_size);
                }
            }
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);


    if (world_rank == master_rank)
    {
        std::vector<int> in_buf(buf_size);

        fs::path base = "data/node";
        fs::create_directories("data/output");
        std::ofstream foutput(fs::current_path() / "oddeven_result.path", std::ofstream::binary);
        // only need one process to merge all sorted segments
        for (int i = 0; i < world_size; ++i)
        {
            fs::path input_path = base / std::to_string(i) / "sorted.bin";
            std::ifstream finput(input_path, std::ifstream::binary);
            if (!finput.is_open())
            {
                cerr << "master failed to open reduce seg " << input_path << endl; 
                exit(1);
            }

            int in_cnt = 0;
            int in_ttl = 0;
            do {
                finput.read(reinterpret_cast<char*>(in_buf.data()), sizeof(dtype) * buf_size);
                in_cnt = finput.gcount() / sizeof(dtype);
                in_ttl += in_cnt;
                foutput.write(reinterpret_cast<char*>(in_buf.data()), sizeof(dtype) * in_cnt);
            } while (in_cnt == buf_size);
            finput.close();
        }
        foutput.close();
    }

    auto end = std::chrono::high_resolution_clock::now();

    if (world_rank == master_rank)
        cout << "time elapsed " << std::chrono::duration<double>(end - start).count() << "s" << endl;

    if (delete_temp && world_rank == master_rank)
    {
        clean_up({
            "data/node",
            "data/temp"
        });
    }

    MPI_Finalize();
    return 0;
}


void scatter_data(
    const char* input_path,
    const char* output_name,
    const int& source_rank
) {
    std::ifstream finput;
    if (world_rank == source_rank)
    {
        // master node is respnsible for reading in the data
        finput.open(input_path, std::ifstream::binary);
        if (!finput.is_open())
        {
            cerr << "failed to open input data bin" << input_path << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    fs::path base_path = "data/node";
    fs::path node_path = std::to_string(world_rank);
    fs::path output_path = base_path / node_path / output_name;
    fs::create_directories(output_path.parent_path()); // return false if dir already exists
    std::ofstream foutput(output_path, std::ofstream::binary);
    if (!foutput.is_open())
    {
        cerr << "failed to open" << output_path << endl;
        MPI_Abort(MPI_COMM_WORLD, 2);
    }

    std::vector<dtype> tx_buf(buf_size);
    std::vector<dtype> rx_buf(buf_size);
    int rx_cnt;
    int tx_cnt;

    do {
        if (world_rank == source_rank)
        {
            finput.read(reinterpret_cast<char*>(tx_buf.data()), sizeof(dtype) * buf_size);
            rx_cnt = finput.gcount() / sizeof(dtype);
        }
        // every node receive signal from main node
        MPI_Bcast(&rx_cnt, 1, MPI_DTYPE, 0, MPI_COMM_WORLD);
        if (rx_cnt == 0) break; // every node exit distribution stage

        tx_cnt = (rx_cnt - 1) / world_size + 1; // divided by each process node, (a-1)/b+1 is ceiling formula
        MPI_Scatter(
            tx_buf.data(), tx_cnt, MPI_DTYPE,
            rx_buf.data(), tx_cnt, MPI_DTYPE,
            source_rank, MPI_COMM_WORLD
        );

        // last node may receive incomplete data
        if (world_rank == world_size - 1)
            tx_cnt = rx_cnt - tx_cnt * (world_size - 1);
        // each node dump the receive data to disk
        foutput.write(reinterpret_cast<char*>(rx_buf.data()), sizeof(dtype) * tx_cnt);
    } while (rx_cnt == buf_size);

    if (world_rank == 0)
        finput.close();
    foutput.close();
}

void internal_sort(
    const char* input_name,
    const char* output_name,
    const int& buf_size
) {
    fs::path base_path = "data/node";
    fs::path node_path = std::to_string(world_rank);

    fs::path input_path = base_path / node_path / input_name;
    fs::path output_path = base_path / node_path / output_name;

    sort_file<dtype>(input_path.c_str(), output_path.c_str(), buf_size, world_rank);
}
