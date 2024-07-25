#include <common_cpp.h>

int main(int argc, char** argv)
{
    std::ifstream finput("data/10K.bin", std::ios::binary);
    if (!finput.is_open())
    {
        fprintf(stderr, "failed to open data bin\n");
        exit(1);
    }

    std::vector<int> buffer(1024 * 11);
    uint byte_need = sizeof(int) * buffer.size();
    finput.read(reinterpret_cast<char*>(buffer.data()), byte_need);
    uint byte_read = finput.gcount();
    if (byte_read < byte_need)
        fprintf(stderr, "read bytes less than specified with actual %ld int data\n", byte_read / sizeof(int));
    finput.close();

    for (const int& num : buffer)
        std::cout << num << ' ';
    
    return 0;
}
