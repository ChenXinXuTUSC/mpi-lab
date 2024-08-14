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
    {
        
    }
    


    if (world_rank == master_rank)
    {
        std::vector<int> in_buf(buf_size);

        fs::path base = "data/node";
        fs::create_directories("data/output");
        std::ofstream foutput("data/output/final.bin", std::ofstream::binary);
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
            tx_buf.data(),
            tx_cnt,
            MPI_DTYPE,
            rx_buf.data(),
            tx_cnt,
            MPI_DTYPE,
            source_rank,
            MPI_COMM_WORLD
        );

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
