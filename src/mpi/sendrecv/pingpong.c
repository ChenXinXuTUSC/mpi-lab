#include <common_c.h>
#include <mpi.h>

const int PP_LIMIT = 10;

int main(int argc, char** argv)
{
    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_size != 2)
    {
        fprintf(stderr, "number of node(proc) must be 2\n");
        exit(1);
    }

    int opponent_rank = (world_rank + 1) % world_size;
    int pp_cnt = 0;

    while (pp_cnt < PP_LIMIT)
    {
        if (world_rank == pp_cnt % 2)
        {
            pp_cnt++;
            MPI_Send(&pp_cnt, 1, MPI_INT, opponent_rank, 0, MPI_COMM_WORLD);
            printf("node %d send pp_cnt %d to %d\n", world_rank, pp_cnt, opponent_rank);
        }
        else
        {
            MPI_Recv(&pp_cnt, 1, MPI_INT, opponent_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("node %d recv pp_cnt %d from %d\n", world_rank, pp_cnt, opponent_rank);
        }
    }

    MPI_Finalize();

    return 0;
}
