SET(
    sub_dir_list

    misc
    mpi
)

SET(cpp_src_list)
SET(c_src_list)

FOREACH(_sub_dir ${sub_dir_list})
    FILE(
        GLOB sub_dir_src_list ${_sub_dir}/*.cpp
    )
    LIST(APPEND cpp_src_list ${sub_dir_src_list})

    FILE(
        GLOB sub_dir_src_list ${_sub_dir}/*.c
    )
    LIST(APPEND c_src_list ${sub_dir_src_list})
ENDFOREACH()



FOREACH(_src_file ${cpp_src_list})
    GET_FILENAME_COMPONENT(_src_name ${_src_file} NAME_WE)
    ADD_EXECUTABLE(${_src_name} ${_src_file})
    TARGET_LINK_LIBRARIES(${_src_name} PRIVATE ${MPI_LIBRARIES} common_cpp)
ENDFOREACH()

FOREACH(_src_file ${c_src_list})
    GET_FILENAME_COMPONENT(_src_name ${_src_file} NAME_WE)
    ADD_EXECUTABLE(${_src_name} ${_src_file})
    TARGET_LINK_LIBRARIES(${_src_name} PRIVATE ${MPI_LIBRARIES} common_c)
ENDFOREACH()
