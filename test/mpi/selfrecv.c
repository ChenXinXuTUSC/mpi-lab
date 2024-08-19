#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int rank, size;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int data = rank;

    // 发送数据到自己
    if (rank != 0)
        MPI_Send(&data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    if (rank == 0)
    {
        for (int i = 1; i < size; ++i)
        {
            MPI_Recv(&data, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            printf("master recv %d from node%d\n", data, i);
        }
    }
    MPI_Finalize();
    return 0;
}
