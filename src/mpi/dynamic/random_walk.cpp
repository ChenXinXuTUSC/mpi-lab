#include <common_c.h>
#include <mpi/mpi.h>

#include <vector>

struct walker
{
    int location;
    int num_step_left;
    walker(){}
    walker(const int loc, const int lft) : location(loc), num_step_left(lft) {}
};

void init_walker(
    const int num_walker_per_proc,
    const int max_walk_dist,
    const int subdomain_start,
    std::vector<walker>* initial_walker_vec 
)
{
    for (int i = 0; i < num_walker_per_proc; ++i)
    {
        initial_walker_vec->push_back(
            walker(subdomain_start, (rand() / (float)RAND_MAX) * max_walk_dist)
        );
    }
}

void walk(
    walker* one_walker,
    const int subdomain_start,
    const int subdomain_sized,
    const int domain_size,
    std::vector<walker>* out_walker_vec
)
{
    while (one_walker->num_step_left > 0)
    {
        if (one_walker->location == subdomain_start + subdomain_sized)
        {
            if (one_walker->location == domain_size)
                one_walker->location = 0;
            out_walker_vec->push_back(*one_walker);
            break;
        }
        else
        {
            one_walker->location++;
            one_walker->num_step_left--;
        }
    }
}

void send_outgoing_walkers(
    std::vector<walker>* outgoing_walker_vec,
    const int world_rank,
    const int world_size
)
{
    // send out data as MPI_BYTE type
    MPI_Send(
        (void*)outgoing_walker_vec->data(),
        outgoing_walker_vec->size() * sizeof(walker),
        MPI_BYTE,
        (world_rank + 1) % world_size, 0, MPI_COMM_WORLD
    );

    outgoing_walker_vec->clear();
}

void recv_incoming_walkers(
    std::vector<walker>* incoming_walker_vec,
    const int world_rank,
    const int world_size
)
{
    MPI_Status status;
    // receive from the node before this, if current node is 0
    // then receive form the last node
    int incoming_rank = (world_rank == 0) ? world_size - 1 : world_rank - 1;
    MPI_Probe(incoming_rank, 0, MPI_COMM_WORLD, &status); // blocking IO

    // resize incoming walker buffer according to the probed info
    int incoming_walker_size;
    MPI_Get_count(&status, MPI_BYTE, &incoming_walker_size);
    incoming_walker_vec->resize(incoming_walker_size / sizeof(walker));
    MPI_Recv(
        (void*)incoming_walker_vec->data(),
        incoming_walker_size,
        MPI_BYTE,
        incoming_rank,
        0,
        MPI_COMM_WORLD,
        MPI_STATUS_IGNORE
    );
}


void decompose_domain(
    const int world_size,
    const int world_rank,
    const int domain_size,
    int* subdomain_start,
    int* subdomain_size
)
{
    if (world_size > domain_size)
    {
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    *subdomain_start = domain_size / world_size * world_rank;
    *subdomain_size  = domain_size / world_size;
    if (world_rank == world_size - 1)
        *subdomain_size += domain_size % world_size;
}

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: random_walk <domain_size> <max_walk_dist> <num_walker_per_node>\n");
        exit(1);
    }
    srand((unsigned int)time(NULL));

    int domain_size = atoi(argv[1]);
    int max_walk_dist = atoi(argv[2]);
    int num_walker_per_proc = atoi(argv[3]);

    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // each process' data variables
    int subdomain_start;
    int subdomain_sized;
    std::vector<walker> incoming_walker_vec;
    std::vector<walker> outgoing_walker_vec;

    decompose_domain(
        world_size, world_rank,
        domain_size,
        &subdomain_start, &subdomain_sized
    );

    init_walker(
        num_walker_per_proc,
        max_walk_dist,
        subdomain_start,
        &incoming_walker_vec
    );

    printf(
        "node %d initiated %d walker(s) in subdomain[%d, %d]\n",
        world_rank,
        num_walker_per_proc,
        subdomain_start, subdomain_start + subdomain_sized - 1
    );


    // simulate the walking process
    int num_max_send_recv = max_walk_dist / (domain_size / world_size) + 1;
    for (int i = 0; i < num_max_send_recv; ++i)
    {
        // process all incoming walkers
        for (walker& one_walker : incoming_walker_vec)
        {
            walk(
                &one_walker,
                subdomain_start,
                subdomain_sized,
                domain_size,
                &outgoing_walker_vec
            );
        }

        printf(
            "node %d send %ld walker(s) to node %d",
            world_rank, outgoing_walker_vec.size(), (world_rank + 1) % world_size
        );

        // prevent dead lock
        if (world_rank % 2 == 1)
        {
            // odd node
            send_outgoing_walkers(&outgoing_walker_vec, world_rank, world_size);
            recv_incoming_walkers(&incoming_walker_vec, world_rank, world_size);
        }
        else
        {
            // even node
            recv_incoming_walkers(&incoming_walker_vec, world_rank, world_size);
            send_outgoing_walkers(&outgoing_walker_vec, world_rank, world_size);
        }

        printf(
            "node %d recv %ld walker(s) from node %d\n",
            world_rank, incoming_walker_vec.size(), ((world_rank - 1 < 0) ? world_size - 1 : world_rank - 1)
        );
    }

    printf("node %d done, exit...\n", world_rank);

    MPI_Finalize();

    return 0;
}
