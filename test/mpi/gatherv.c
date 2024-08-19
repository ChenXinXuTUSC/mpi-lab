#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int rank, size;
    int *sendbuf, *recvbuf;
    int sendcount, *recvcounts, *displs;
    int i;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // 每个进程发送不同数量的数据
    sendcount = rank + 1;  // 进程 rank 发送 rank + 1 个数据

    // 分配发送缓冲区并填充数据
    sendbuf = (int *)malloc(sendcount * sizeof(int));
    for (i = 0; i < sendcount; i++) {
        sendbuf[i] = rank * 10 + i;  // 数据填充示例
    }

    if (rank == 0) {
        // 主进程需要收集数据
        recvcounts = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));

        // 设置每个进程发送的数据量
        for (i = 0; i < size; i++) {
            recvcounts[i] = i + 1;
            displs[i] = (i * (i + 1)) / 2;  // 计算每个进程数据在接收缓冲区中的起始位置
        }

        // 总数据量
        int total_count = (size * (size + 1)) / 2;

        // 分配接收缓冲区
        recvbuf = (int *)malloc(total_count * sizeof(int));
    }

    // 执行 MPI_Gatherv
    MPI_Gatherv(
        sendbuf, sendcount, MPI_INT, 
        recvbuf, recvcounts, displs, MPI_INT, 
        0, MPI_COMM_WORLD
    );

    if (rank == 0) {
        // 主进程输出接收到的数据
        printf("Received data:\n");
        for (i = 0; i < (size * (size + 1)) / 2; i++) {
            printf("%d ", recvbuf[i]);
        }
        printf("\n");

        // 释放主进程的缓冲区
        free(recvcounts);
        free(displs);
        free(recvbuf);
    }

    // 释放各进程的缓冲区
    free(sendbuf);

    MPI_Finalize();
    return 0;
}
