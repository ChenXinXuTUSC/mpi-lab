SET(
    predata_src_list
    txdata.c
    rxdata.c
    cxdata.c
)

FOREACH(_src_file ${predata_src_list})
    GET_FILENAME_COMPONENT(_src_name ${_src_file} NAME_WE)
    ADD_EXECUTABLE(${_src_name} ${_src_file})
    TARGET_LINK_LIBRARIES(${_src_name} PRIVATE common_c)
ENDFOREACH()
