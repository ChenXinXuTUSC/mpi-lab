#include <common_cpp.h>

#ifdef USE_INT
    typedef int dtype;
    #define MPI_DTYPE MPI_INT
#endif

#ifdef USE_FLT
    typedef float dtype;
    #define MPI_DTYPE MPI_FLOAT
#endif


int main(int argc, char** argv)
{
    std::filesystem::path input_path = "data/FLT10.bin";

    std::vector<dtype> data(10);
    std::ifstream finput1(input_path, std::ifstream::binary); finput1.seekg(sizeof(dtype) * 0, std::ifstream::beg);
    std::ifstream finput2(input_path, std::ifstream::binary); finput2.seekg(sizeof(dtype) * 4, std::ifstream::beg);
    std::ifstream finput3(input_path, std::ifstream::binary); finput3.seekg(sizeof(dtype) * 8, std::ifstream::beg);

    finput1.read(reinterpret_cast<char*>(data.data() + 0), sizeof(dtype) * 4);
    finput2.read(reinterpret_cast<char*>(data.data() + 4), sizeof(dtype) * 4);
    finput3.read(reinterpret_cast<char*>(data.data() + 8), sizeof(dtype) * 2);

    finput1.close();
    finput2.close();
    finput3.close();

    for (const dtype& dd : data) cout << dd << ' ';
    return 0;
}
