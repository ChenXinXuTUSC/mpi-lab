#include <common_c.h>

#include <mpi.h>

int main(int argc, char** argv)
{
    // initialize MPI environment
    MPI_Init(NULL, NULL);

    // get world size
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // get node rank
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int number;

    if (world_rank == 0)
    {
        // master node send data to slave node
        number = -1;
        MPI_Send(
            &number,        // data address
            1,              // data amount
            MPI_INT,        // data type
            1,              // destination node rank
            0,              // message type tag
            MPI_COMM_WORLD  // current communication world
        );
        printf("node %d send: %d\n", world_rank, number);
    }
    else if (world_rank == 1)
    {
        // slave node receive data
        MPI_Recv(
            &number,          // data address
            1,                // data amount
            MPI_INT,          // data type
            0,                // source node rank
            0,                // message type tag
            MPI_COMM_WORLD,   // current communication world
            MPI_STATUS_IGNORE // status
        );
        printf("node %d recv: %d\n", world_rank, number);
    }

    MPI_Finalize();

    return 0;
}
