#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int rank, size;
    int *send_data = NULL;
    int *recv_data = NULL;
    int sendcount, recvcount;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Each process sends `world_size` integers
    sendcount = size;
    send_data = (int*)malloc(sendcount * sizeof(int));
    for (int i = 0; i < sendcount; i++) {
        send_data[i] = rank * 10 + i;  // Data to send
    }

    // Root process will gather `world_size - 1` item from each process
    if (rank == 0) {
        recvcount = size;  // Root process will gather 1 item from each process
        recv_data = (int*)malloc(size * size * sizeof(int));
    } else {
        recvcount = 0;  // Non-root processes do not use recv_data
    }

    // Gather the first item from each process (only root process will collect data)
    MPI_Gather(
        send_data, sendcount, MPI_INT,
        recv_data, recvcount, MPI_INT,
        0, MPI_COMM_WORLD
    );

    // Root process prints the gathered data
    if (rank == 0) {
        printf("Root process gathered data: ");
        for (int i = 0; i < size * size; i++) {
            printf("%d ", recv_data[i]);
        }
        printf("\n");
    }

    free(recv_data);
    free(send_data);
    MPI_Finalize();
    return 0;
}
