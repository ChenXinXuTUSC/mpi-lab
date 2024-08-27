#include <common_cpp.h>


void parse_args(
    int argc, char** argv,
    const char* shortopts,
    void(*args_handler)(
        const int opt,
        const int optopt,
        const int optind,
        char* optarg
    )
)
{
    extern char* optarg;
    extern int   optopt;
    extern int   optind;

    int opt;
    while ((opt = getopt(argc, argv, shortopts)) != -1)
    {
        args_handler(opt, optopt, optind, optarg);
    }
}


void clean_up(std::vector<std::string> dirs)
{
    for (const std::string& dir_path : dirs)
    {
        try
        {
            if (std::filesystem::exists(dir_path))
                std::filesystem::remove_all(dir_path);
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            std::cerr << "remove temporary files error: " << e.what() << std::endl;
        }
    }
}

void c_truncate(
    const char* file_path,
    const size_t typesz,
    const size_t offset,
    const size_t movesz,
    const size_t buffsz
) {
    if (buffsz <= 0 || typesz <= 0)
    {
        printf("invalid arguments buffsz=%ld, typesz=%ld\n", buffsz, typesz);
        exit(-1);
    }

    FILE* rx_ptr = fopen(file_path, "r+");
    if (rx_ptr == NULL)
    {
        printf("failed to open file %s\n", file_path);
        exit(1);
    }

    FILE* tx_ptr = fopen(file_path, "r+");
    if (tx_ptr == NULL)
    {
        printf("failed to open file %s\n", file_path);
        exit(1);
    }

    char* buf = (char*)malloc(typesz * buffsz);
    if (buf == NULL)
    {
        printf("failed to allocate buffer with size %ld\n", buffsz);
        exit(3);
    }

    if (fseek(rx_ptr, (long)offset * (long)typesz, SEEK_SET))
    {
        perror("failed to fseek rx_ptr");
        printf("fseek %ld * %ld\n", offset, typesz);
        exit(1);
    }
    if (fseek(tx_ptr, 0, SEEK_SET))
    {
        perror("failed to fseek tx_ptr");
        exit(1);
    }

    size_t movect = 0;
    size_t rx_cnt = 0;
    do {
        rx_cnt = fread(buf, typesz, min(buffsz, movesz - movect), rx_ptr);
        // printf("%s\n", buf);
        fwrite(buf, typesz, rx_cnt, tx_ptr);

        movect += rx_cnt;
    } while (movect < movesz && rx_cnt == buffsz);


    fseek(tx_ptr, 0, SEEK_SET);
    if (ftruncate(fileno(tx_ptr), (long)movect * long(typesz)) != 0)
    {
        perror("failed to truncate result file");
        printf("offset %ld * %ld\n", movect, typesz);
        exit(2);
    }

    fclose(rx_ptr);
    fclose(tx_ptr);
    free(buf);
}
