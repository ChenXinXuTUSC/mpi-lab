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

char processor_name[MPI_MAX_PROCESSOR_NAME];
int processor_name_len;

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
    MPI_Get_processor_name(processor_name, &processor_name_len);

    timer timer_io("node" + std::to_string(world_rank) + " io", true); // input & output time
    timer timer_ex("node" + std::to_string(world_rank) + " ex", true); // sort execution time
    timer timer_st("node" + std::to_string(world_rank) + " st", true); // stage time

    fs::path log_path = fs::path("log") / "node" / std::to_string(world_rank) / "run.log";
    fs::create_directories(log_path.parent_path());
    std::ofstream flogout(log_path, std::ofstream::trunc);
    if (!flogout.is_open())
    {
        cerr << "node" << world_rank << " failed to open log file" << endl;
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    // step1: distribute data to all nodes
    timer_io.tick();
    scatter_data(bin_data_path, "recv.bin", master_rank);
    MPI_Barrier(MPI_COMM_WORLD); // end of data distribution
    timer_io.tock("data distribution");
    
    // step2: each proc sort its segment
    timer_ex.tick();
    internal_sort("recv.bin", "sorted.bin", buf_size);
    MPI_Barrier(MPI_COMM_WORLD); // end of data each node file sort
    timer_ex.tock("segment internal sort");

    // step3: each node perform regular sampling
    timer_st.tick();
    std::vector<dtype> sample_list;
    {
        int send_cnt = 0; // record each node's send count

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
        for (int i = 0; i < world_size; ++i)
        {
            size_t idx = (item_cnt / world_size) * i;
            finput.seekg(idx * sizeof(dtype), std::ios::beg);
            finput.read(reinterpret_cast<char*>(&temp_data), sizeof(dtype) * 1);
            sample_list.emplace_back(temp_data);
        }
        finput.close();
    }
    timer_st.tock("regular sampling");
    MPI_Barrier(MPI_COMM_WORLD); // end of regular sampling

    // step4: master gather all node's sample result
    std::vector<dtype> all_sample;
    timer_io.tick();
    {
        int send_cnt = sample_list.size();

        // get count from each slave node
        std::vector<int> all_sample_cnt;
        std::vector<int> all_sample_off;

        // every node other than master send its count
        if (world_rank != master_rank)
            MPI_Send(&send_cnt, 1, MPI_INT, master_rank, 0, MPI_COMM_WORLD);
        // only master gathers all other nodes' count
        if (world_rank == master_rank)
        {
            all_sample_cnt.resize(world_size);
            all_sample_off.resize(world_size);
            all_sample_cnt[master_rank] = send_cnt;

            for (int i = 0; i < world_size; ++i)
            {
                all_sample_off[i] = i * world_size; // set offset
                if (i == master_rank) continue;
                // set count
                MPI_Recv(&all_sample_cnt[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
        if (world_rank == master_rank) all_sample.resize(world_size * world_size);
        MPI_Gatherv(
            sample_list.data(), world_size, MPI_DTYPE,
            all_sample.data(), all_sample_cnt.data(), all_sample_off.data(), MPI_INT,
            master_rank, MPI_COMM_WORLD
        );
    }
    if (world_rank == master_rank)
    {
        // cout << "node" << master_rank << " receive samples:\n";
        // for (const dtype& sample : all_sample)
        //     cout << sample << ' ';
        std::sort(all_sample.begin(), all_sample.end());

        // generate real sample
        std::vector<dtype> temp_sample;
        for (int i = 1; i < world_size; ++i)
            temp_sample.emplace_back(all_sample[(all_sample.size() / world_size) * i]);
        all_sample = temp_sample;
        // for (const dtype& dd : all_sample) cout << dd << ' ';
        // cout << endl;
    }
    timer_io.tock("exchange reguler pivot");

    // step 5: broadcast pivot to every node
    std::vector<dtype> pivot_list;
    if (world_rank == master_rank)
        pivot_list = all_sample;
    else
        pivot_list.resize(world_size - 1);
    MPI_Bcast(pivot_list.data(), pivot_list.size(), MPI_DTYPE, master_rank, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD); // end of regular sampling
    // cout << "node" << world_rank << " receive samples: ";
    // for (const dtype& dd : pivot_list) cout << dd << ' ';
    // cout << endl;
    // MPI_Finalize();
    // return 0;


    // step6: each node send corresponding segment to other corresponding nodes
    timer_io.tick();
    {
        fs::path base = "data/node";

        fs::path input_path = base / std::to_string(world_rank) / "sorted.bin";
        std::ifstream finput(input_path, std::ifstream::binary);
        if (!finput.is_open())
        {
            cerr << "node" << world_rank << " failed to open " << input_path << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // prepare read head for each segment
        std::vector<std::shared_ptr<std::ifstream>> read_headp_list;
        std::vector<unsigned int> all_send_tlt(world_size);
        int pivot_idx = 0;
        dtype hold;
        size_t recv_cnt = 0;
        size_t recv_tlt = 0;
        do {
            finput.read(reinterpret_cast<char*>(&hold), sizeof(dtype) * 1);
            recv_cnt = finput.gcount() / sizeof(dtype);
            if (recv_cnt <= 0) break;
            recv_tlt++; // each time only 1 data is read in

            if (pivot_idx < world_size - 1 && hold > pivot_list[pivot_idx])
            {
                // enter the next segment, before this operation, prepare
                // the head for the current segment
                std::shared_ptr<std::ifstream> headp = std::make_shared<std::ifstream>(input_path, std::ifstream::binary);
                headp->seekg((recv_tlt - all_send_tlt[pivot_idx]) * sizeof(dtype), std::ifstream::beg);
                read_headp_list.emplace_back(headp); // no copy constructor but move constructor

                pivot_idx++;
            }
            all_send_tlt[pivot_idx]++;
        } while (recv_cnt > 0);
        // don't forget the last segment
        {
            std::shared_ptr<std::ifstream> headp = std::make_shared<std::ifstream>(input_path, std::ifstream::binary);
            headp->seekg((recv_tlt - all_send_tlt[pivot_idx]) * sizeof(dtype), std::ifstream::beg);
            read_headp_list.push_back(headp);
        }
        finput.close();



        // broadcast the total receive amount to each node
        std::vector<unsigned int> all_recv_tlt(world_size);
        MPI_Alltoall(
            all_send_tlt.data(), 1, MPI_INT,
            all_recv_tlt.data(), 1, MPI_INT,
            MPI_COMM_WORLD
        );

        unsigned int max_seg_len = buf_size / world_size;
        std::vector<dtype> send_buf(buf_size);
        std::vector<dtype> recv_buf(buf_size);
        std::vector<int> all_send_cnt(world_size);
        std::vector<int> all_recv_cnt(world_size);
        std::vector<int> buf_offset(world_size);
        for (int i = 0; i < world_size; ++i)
            buf_offset[i] = i * max_seg_len;

        fs::path seg_dir = base / std::to_string(world_rank) / "seg";
        fs::remove_all(seg_dir); // in case some other function create files with same name
        fs::create_directories(seg_dir);
        if (!fs::exists(seg_dir))
        {
            cerr << "node" << world_rank << " failed to create segment dir\n";
            MPI_Abort(MPI_COMM_WORLD, 2);
        }

        int round = 1;
        do {
            std::fill(all_send_cnt.begin(), all_send_cnt.end(), 0);
            std::fill(all_recv_cnt.begin(), all_recv_cnt.end(), 0);
            // prepare for MPI_Alltoallv
            // different nodes may have different amount of segments

            // prepare for send 
            for (int i = 0; i < read_headp_list.size(); ++i)
            {
                all_send_cnt[i] = min(max_seg_len, all_send_tlt[i]);
                all_send_tlt[i] -= all_send_cnt[i];
                read_headp_list[i]->read(
                    reinterpret_cast<char*>(send_buf.data() + i * max_seg_len), // buffer location
                    sizeof(dtype) * all_send_cnt[i] // amount of bytes to be read
                );
            }
            // prepare for recv
            MPI_Alltoall(
                all_send_cnt.data(), 1, MPI_INT,
                all_recv_cnt.data(), 1, MPI_INT,
                MPI_COMM_WORLD
            );
            // flogout << "[round " << round << "] node" << world_rank << " send:";
            // for (int i = 0; i < world_size; ++i) flogout << all_send_cnt[i] << '/' << all_send_tlt[i] << ' ';
            // flogout << "total " << std::accumulate(all_recv_cnt.begin(), all_recv_cnt.end(), 0) << endl;
            // flogout << "[round " << round << "] node" << world_rank << " recv:";
            // for (const int& x : all_recv_cnt) flogout << x << ' ';
            // flogout << "total " << std::accumulate(all_recv_cnt.begin(), all_recv_cnt.end(), 0) << endl;
            
            // perform scatter and gather
            MPI_Alltoallv(
                send_buf.data(), all_send_cnt.data(), buf_offset.data(), MPI_DTYPE,
                recv_buf.data(), all_recv_cnt.data(), buf_offset.data(), MPI_DTYPE,
                MPI_COMM_WORLD
            );

            
            // dump to the corresponding segment
            for (int i = 0; i < world_size; ++i)
            {
                std::ofstream foutput(seg_dir / (std::to_string(i) + ".bin"), std::ofstream::app | std::ofstream::binary);
                if (!foutput.is_open())
                {
                    cerr << "node" << world_rank << " failed to open segment file";
                    MPI_Abort(MPI_COMM_WORLD, 2);
                }
                foutput.write(
                    reinterpret_cast<char*>(recv_buf.data() + i * max_seg_len),
                    sizeof(dtype) * all_recv_cnt[i]
                );
                // flogout << "[round " << round << "] node" << world_rank << " dump seg" << i << " : " << all_recv_cnt[i] << endl;
                foutput.close();
            }

            round++;
        } while (
            // no more data to send or recv
            std::accumulate(all_send_cnt.begin(), all_send_cnt.end(), 0) > 0 ||
            std::accumulate(all_recv_cnt.begin(), all_recv_cnt.end(), 0) > 0
        );
    }
    timer_io.tock("MPI_Alltoallv exchange segments");
    MPI_Barrier(MPI_COMM_WORLD);

    // step 7: perform kmerge file on segments
    timer_ex.tick();
    {
        size_t file_size_total = 0;

        fs::path base = "data/node";
        fs::path seg_dir = base / std::to_string(world_rank) / "seg";
        if (!fs::exists(seg_dir) || !fs::is_directory(seg_dir))
        {
            cerr << "node" << world_rank << " not found segment dir" << endl;
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        std::vector<std::string> input_file_list;
        for (const auto& entry : fs::directory_iterator(seg_dir)) {
            if (fs::is_regular_file(entry.status())) {
                // cout << "node" << world_rank << " segment: " << entry.path() << endl;
                input_file_list.emplace_back(entry.path().c_str());
                file_size_total += fs::file_size(entry.path());
            }
        }

        // flogout << "segment sort size:" << endl;
        // flogout << "B="<< file_size_total << endl;
        // flogout << "KB="<< file_size_total / (size_t)pow(2, 10) << endl;
        // flogout << "MB="<< file_size_total / (size_t)pow(2, 20) << endl;
        // flogout << "GB="<< file_size_total / (size_t)pow(2, 30) << endl;

        fs::path output_file_path = base / "sorted.bin";
        kmerge_file<dtype>(input_file_list, output_file_path.c_str());
    }
    timer_ex.tock("pivoted segment internal sort");
    MPI_Barrier(MPI_COMM_WORLD);

    // step 8: master node gathers all sorted segments
    // if (world_rank == master_rank)
    // {
    //     timer_io.tick();
    //     fs::path base = "data/node";
    //     if (!fs::exists(base) || !fs::is_directory(base))
    //     {
    //         cerr << "master" << world_rank << " not found base dir" << endl;
    //         MPI_Abort(MPI_COMM_WORLD, -1);
    //     }
    //     std::vector<std::string> input_file_list;
    //     for (int i = 0; i < world_size; ++i)
    //     {
    //         fs::path seg_sorted_path = base / std::to_string(i) / "sorted.bin";
    //         if (!fs::exists(seg_sorted_path))
    //         {
    //             cerr << "master" << world_rank << " not found sorted segment " << i << endl;
    //             MPI_Abort(MPI_COMM_WORLD, -1);
    //         }
    //         input_file_list.emplace_back(seg_sorted_path.c_str());
    //     }
    //     fs::path output_file_path = fs::current_path() / "psrs_result.bin";
    //     kmerge_file<dtype>(input_file_list, output_file_path.c_str());
    //     timer_io.tock("master gather all sorted segments");
    // }

    if (world_rank == master_rank)
    {
        // output time count statistic
        auto io_duration_list = timer_io.get_duration_list();
        auto io_caption_list = timer_io.get_caption_lits();
        double io_total = timer_io.total_count();
        flogout << "[io stages]" << endl;
        for (int i = 0; i < io_duration_list.size(); ++i)
        {
            flogout << std::fixed << std::setprecision(2);
            flogout << std::setw(10) << io_duration_list[i] / io_total * 100.0 << '%';
            flogout.unsetf(std::ios::fixed);
            flogout << std::setw(10) << io_duration_list[i] << 's';
            flogout << " " << io_caption_list[i] << endl;
        }

        auto ex_duration_list = timer_ex.get_duration_list();
        auto ex_caption_list = timer_ex.get_caption_lits();
        double ex_total = timer_ex.total_count();
        flogout << "[ex stages]" << endl;
        for (int i = 0; i < ex_duration_list.size(); ++i)
        {
            flogout << std::fixed << std::setprecision(2);
            flogout << std::setw(10) << ex_duration_list[i] / ex_total * 100.0 << '%';
            flogout.unsetf(std::ios::fixed);
            flogout << std::setw(10) << ex_duration_list[i] << 's';
            flogout << " "<< ex_caption_list[i] << endl;
        }
    }


    if (delete_temp && world_rank == master_rank)
    {
        clean_up({
            "data/node",
            "data/temp"
        });
    }


    MPI_Finalize();
    flogout.close();
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
