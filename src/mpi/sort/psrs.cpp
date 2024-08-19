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

    // step1: distribute data to all nodes
    scatter_data(bin_data_path, "recv.bin", master_rank);
    MPI_Barrier(MPI_COMM_WORLD); // end of data distribution


    // step2: each proc sort its segment
    internal_sort("recv.bin", "sorted.bin", buf_size);
    MPI_Barrier(MPI_COMM_WORLD); // end of data each node file sort

    // step3: each node perform regular sampling
    std::vector<dtype> sample_list;
    {
        int tx_cnt = 0; // record each node's send count

        fs::path base = "data/node";
        fs::path input_path = base / std::to_string(world_rank) / "sorted.bin";
        std::ifstream finput(input_path, std::ifstream::binary);
        if (!finput.is_open())
        {
            cerr << "node" << world_rank << " failed to open " << input_path << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        dtype temp_data;
        size_t item_cnt = fs::file_size(input_path) / sizeof(dtype);
        // cout << "node" << world_rank << " sampling: ";
        for (int i = 0; i < world_size; ++i)
        {
            size_t idx = (item_cnt / world_size) * i;
            finput.seekg(idx * sizeof(dtype), std::ios::beg);
            finput.read(reinterpret_cast<char*>(&temp_data), sizeof(dtype) * 1);
            sample_list.emplace_back(temp_data);
            // cout << temp_data << ' ';
        }
        // cout << endl;
        finput.close();
    }

    // step4: master gather all node's sample result
    std::vector<dtype> all_sample;
    {
        int tx_cnt = sample_list.size();

        // get count from each slave node
        std::vector<int> all_sample_cnt;
        std::vector<int> all_sample_off;
        {
            // every node other than master send its count
            if (world_rank != master_rank)
                MPI_Send(&tx_cnt, 1, MPI_INT, master_rank, 0, MPI_COMM_WORLD);
            // only master gathers all other nodes' count
            if (world_rank == master_rank)
            {
                all_sample_cnt.resize(world_size);
                all_sample_off.resize(world_size);

                all_sample_cnt[master_rank] = tx_cnt;
                for (int i = 0; i < world_size; ++i)
                {
                    all_sample_off[i] = i * world_size; // set offset
                    if (i == master_rank) continue;
                    // set count
                    MPI_Recv(&all_sample_cnt[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }
        }
        if (world_rank == master_rank) all_sample.resize(world_size * world_size);
        MPI_Gatherv(
            sample_list.data(), world_size, MPI_DTYPE,
            all_sample.data(), all_sample_cnt.data(), all_sample_off.data(), MPI_INT,
            master_rank, MPI_COMM_WORLD
        );
    }
    MPI_Barrier(MPI_COMM_WORLD); // end of regular sampling
    if (world_rank == master_rank)
    {
        cout << "node" << master_rank << " receive samples:\n";
        for (const dtype& sample : all_sample)
            cout << sample << ' ';
    }

    // master node select pivots and broadcast
    // {
    //     std::vector<dtype> rs_list;
    //     rs_list.resize(world_size);
    //     if (world_rank == master_rank)
    //     {
    //         std::sort(all_sample.begin(), all_sample.end());
    //         for (int i = 1; i < world_size; ++i)
    //             rs_list[(all_sample.size() / world_size) * i];
    //         MPI_Bcast(rs_list.data(), world_size, MPI_DTYPE, master_rank, MPI_COMM_WORLD);
    //     }
    // }
    // MPI_Barrier(MPI_COMM_WORLD);


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
    fs::path output_path = base_path / std::to_string(world_rank) / output_name;
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

    fs::path input_path = base_path / std::to_string(world_rank) / input_name;
    fs::path output_path = base_path / std::to_string(world_rank) / output_name;

    sort_file<dtype>(input_path.c_str(), output_path.c_str(), buf_size, world_rank);
}
