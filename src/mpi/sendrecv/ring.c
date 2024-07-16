#include <common_c.h>
#include <mpi.h>

int main(int argc, char** argv)
{
    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int token = 0;

    if (world_rank != 0)
    {
        // receive from lower node to higher node
        MPI_Recv(&token, 1, MPI_INT, world_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("node %d recv token: %d\n", world_rank, token);
    } else {
        // master node set token to -1
        token = -1;
    }
    token++;
    MPI_Send(&token, 1, MPI_INT, (world_rank + 1) % world_size, 0, MPI_COMM_WORLD);

    // receive from the last node to form a ring
    if (world_rank == 0)
    {
        MPI_Recv(&token, 1, MPI_INT, world_size - 1, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
        printf("node 0 recv token: %d\n", token);
    }

    MPI_Finalize();

    return 0;
}
