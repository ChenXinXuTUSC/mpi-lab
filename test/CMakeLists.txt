SET(
    test_src_list

    misc/reftest.cpp
    misc/lessread.cpp
    misc/sort.cpp
    misc/ifstreamsptr.cpp
)

FOREACH(_src_file ${test_src_list})
    GET_FILENAME_COMPONENT(_src_name ${_src_file} NAME_WE)
    ADD_EXECUTABLE(${_src_name} ${_src_file})
    TARGET_LINK_LIBRARIES(${_src_name} PRIVATE ${MPI_LIBRARIES})
ENDFOREACH()
