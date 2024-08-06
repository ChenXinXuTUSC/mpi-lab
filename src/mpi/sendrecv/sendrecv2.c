#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int send_data[5]; // 数据发送缓冲区
    int recv_data[5]; // 数据接收缓冲区
    MPI_Status status;
    int rx_cnt;

    if (rank == 0) {
        // 进程0发送3个整数
        send_data[0] = 1;
        send_data[1] = 2;
        send_data[2] = 3;

        MPI_Sendrecv(
            send_data, 0, MPI_INT, 1, 0, 
            recv_data, 5, MPI_INT, 1, 0, 
            MPI_COMM_WORLD, &status
        );
        MPI_Get_count(&status, MPI_INT, &rx_cnt);
        printf("node0 recv %d data\n", rx_cnt);
        for (int i = 0; i < rx_cnt; i++) {
            printf("node0 data %d\n", recv_data[i]);
        }
    } else if (rank == 1) {
        // 进程1发送5个整数
        send_data[0] = 4;
        send_data[1] = 5;
        send_data[2] = 6;
        send_data[3] = 7;
        send_data[4] = 8;

        MPI_Sendrecv(
            send_data, 5, MPI_INT, 0, 0, 
            recv_data, 5, MPI_INT, 0, 0, 
            MPI_COMM_WORLD, &status
        );
        MPI_Get_count(&status, MPI_INT, &rx_cnt);
        printf("node1 recv %d data\n", rx_cnt);
        for (int i = 0; i < rx_cnt; i++) {
            printf("node1 %d\n", recv_data[i]);
        }
        printf("\n");
    }

    MPI_Finalize();
    return 0;
}
