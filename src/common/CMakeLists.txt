FILE(
    GLOB common_cpp_src_list
    cpp/*.cpp
)
ADD_LIBRARY(
    common_cpp
    STATIC
    ${common_cpp_src_list}
)

FILE(
    GLOB common_c_src_list
    c/*.c
)
ADD_LIBRARY(
    common_c
    STATIC
    ${common_c_src_list}
)
