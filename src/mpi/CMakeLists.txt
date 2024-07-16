SET(
    mpi_src_list
    sendrecv/helloworld.c
    sendrecv/sendrecv.c
    sendrecv/pingpong.c
    sendrecv/ring.c

    dynamic/check_status.c
    dynamic/random_walk.cpp

    collective/mybcast.c
    collective/avg.c

    sort/oddevensort.c
    sort/kmerge.cpp
)

FOREACH(_src_file ${mpi_src_list})
    GET_FILENAME_COMPONENT(_src_name ${_src_file} NAME_WE)
    ADD_EXECUTABLE(${_src_name} ${_src_file})
    TARGET_LINK_LIBRARIES(${_src_name} PRIVATE ${MPI_LIBRARIES})
ENDFOREACH()
