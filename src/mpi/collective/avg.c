#include <common_c.h>
#include <mpi.h>

float* gen_data_array(const int num_ele)
{
    float* array = (float*)malloc(sizeof(float) * num_ele);
    
    assert(array != NULL);

    for (int i = 0; i < num_ele; ++i)
        array[i] = (rand() / (float)RAND_MAX);

    return array;
}

float calc_avg(float* array, const int num_ele)
{
    float sum = 0.0f;
    for (int i = 0; i < num_ele; ++i)
        sum += array[i];
    return sum / (float)num_ele;
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: avg <num_per_node>\n");
        exit(1);
    }
    int num_per_node = atoi(argv[1]);

    srand((unsigned int)time(NULL));

    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    float* array_all = NULL;
    if (world_rank == 0)
    {
        array_all = gen_data_array(num_per_node * world_size);
        printf("result of serial: %.2f\n", calc_avg(array_all, num_per_node * world_size));
    }

    float* array_sub = NULL;
    array_sub = (float*)malloc(sizeof(float) * num_per_node);

    // distribute data to all nodes
    MPI_Scatter(
        array_all,
        num_per_node,
        MPI_FLOAT,
        array_sub,
        num_per_node,
        MPI_FLOAT,
        0,
        MPI_COMM_WORLD
    );

    // each node compute the average of its data
    float avg_sub = calc_avg(array_sub, num_per_node);

    // gather all partial average
    float* array_avg = NULL;
    if (world_rank == 0)
    {
        array_avg = (float*)malloc(sizeof(float) * world_size);
        assert(array_avg != NULL);
    }
    MPI_Gather(&avg_sub, 1, MPI_FLOAT, array_avg, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);

    if (world_rank == 0)
    {
        float avg = calc_avg(array_avg, world_size);
        printf("result of concurrent: %.2f\n", avg);
    }

    free(array_all);
    free(array_sub);
    free(array_avg);
    MPI_Finalize();
    return 0;
}
