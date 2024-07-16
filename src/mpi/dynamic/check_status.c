#include <common_c.h>
#include <mpi.h>

#include <time.h>

int main(int argc, char** argv)
{
    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_size != 2)
    {
        fprintf(stderr, "number of node must be 2 for this example\n");
        exit(1);
    }

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int array[100]; // random data bytes

    if (world_rank == 0)
    {
        // master node send random amount of number
        srand((unsigned int)time(NULL));
        int amount = (rand() / (float)RAND_MAX) * 100;
        MPI_Send(array, amount, MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("node 0 send %d numbers to node 1\n",  amount);
    }
    else if (world_rank == 1)
    {
        MPI_Status status;
        int amount;
        MPI_Recv(array, 100, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); // receive at most 100 numbers
        MPI_Get_count(&status, MPI_INT, &amount);
        printf("node 1 recv %d numbers from node 0\n", amount);
    }
    MPI_Barrier(MPI_COMM_WORLD); // stop master node to wait for slave node
    MPI_Finalize();

    return 0;
}
