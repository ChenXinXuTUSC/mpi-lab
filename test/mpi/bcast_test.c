#include <common_c.h>
#include <mpi/mpi.h>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int data;

    if (world_rank == 0)
    {
        data = 1;
    }
    MPI_Bcast(&data, 1, MPI_INT, 0, MPI_COMM_WORLD);

    printf("node#%d has data %d\n", world_rank, data);

    MPI_Finalize();
    return 0;
}
