#include <common_cpp.h>
#include <filesystem>
#include <unistd.h>

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <file_path> <truncate_size>\n", argv[0]);
        exit(-1);
    }

    int truncate_size = atoi(argv[2]);
    if (truncate_size <= 0)
    {
        fprintf(stderr, "invalid truncate size %s\n", argv[2]);
        exit(-1);
    }

    if (!std::filesystem::exists(argv[1]))
    {
        fprintf(stderr, "failed to open %s\n", argv[1]);
        exit(1);
    }

    std::filesystem::resize_file(argv[1], truncate_size);

    return 0;
}
