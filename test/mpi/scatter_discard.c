#include <common_c.h>
#include <mpi.h>

#include <getopt.h>
#include <ctype.h>

// global data
int array_size = 0;

void args_handler(
    const int opt,
    const int optopt,
    const int optind,
    char* optarg
) {
    switch(opt)
    {
    case 'n':
        array_size = atoi(optarg);
        if (array_size <= 0)
        {
            fprintf(stderr, "invalid array size %d\n", array_size);
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
    parse_args(argc, argv, "n:", &args_handler);

    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    float* send_data = NULL;
    if (world_rank == 0)
    {
        // only master node need to initialize data
        send_data = (float*)malloc(sizeof(float) * array_size);
        for (int i = 0; i < array_size; ++i)
            send_data[i] = (float)(i + 1);
    }

    float* recv_data = NULL;
    int recv_size = 0;
    // int recv_size = (array_size - 1) / world_size + 1;
    if (world_rank == 0)
        recv_size = (array_size - 1) / world_size + 1;
    
    recv_data = (float*)malloc(sizeof(float) * recv_size); // ceiling

    MPI_Scatter(
        send_data,
        recv_size, // ceiling
        MPI_FLOAT,
        recv_data,
        recv_size,
        MPI_FLOAT,
        0,
        MPI_COMM_WORLD
    );

    recv_size = (array_size - 1) / world_size + 1;
    printf("node#%d: ", world_rank);
    for (int i = 0; i < recv_size; ++i)
        printf("%.2f ", recv_data[i]);
    printf("\n");

    free(send_data);
    free(recv_data);
    MPI_Finalize();
    return 0;
}
