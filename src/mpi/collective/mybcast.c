#include <common_c.h>
#include <mpi.h>

void mybcast(void* dataaddr, const int count, MPI_Datatype datatype, const int rootnode, MPI_Comm world)
{
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_rank == rootnode)
    {
        for (int nextnode = 0; nextnode < world_size; ++nextnode)
        {
            if (nextnode != world_rank)
            {
                MPI_Send(dataaddr, count, datatype, nextnode, 0, world);
            }
        }
    }
    else
    {
        MPI_Recv(dataaddr, count, datatype, rootnode, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

int main(int argc, char** argv)
{
    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);


    int data;
    if (world_rank == 0)
    {
        data = 100;
        printf("node 0 broadcast %d to world\n", data);
        mybcast(&data, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
    else
    {
        mybcast(&data, 1, MPI_INT, 0, MPI_COMM_WORLD);
        printf("node %d recv data %d\n", world_rank, data);
    }

    MPI_Finalize();

    return 0;
}
