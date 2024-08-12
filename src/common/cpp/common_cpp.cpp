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


void c_truncate(
    const char* file_path,
    const int typesz,
    const int offset,
    const int movesz,
    const int buffsz
) {
    if (buffsz <= 0 || typesz <= 0)
    {
        fprintf(stderr, "invalid arguments buffsz=%d, typesz=%d\n", buffsz, typesz);
        exit(-1);
    }

    FILE* rx_ptr = fopen(file_path, "r+");
    if (rx_ptr == NULL)
    {
        fprintf(stderr, "failed to open file %s\n", file_path);
        exit(1);
    }

    FILE* tx_ptr = fopen(file_path, "r+");
    if (tx_ptr == NULL)
    {
        fprintf(stderr, "failed to open file %s\n", file_path);
        exit(1);
    }

    char* buf = (char*)malloc(typesz * buffsz);
    if (buf == NULL)
    {
        fprintf(stderr, "failed to allocate buffer with size %d\n", buffsz);
        exit(3);
    }

    if (fseek(rx_ptr, offset * typesz, SEEK_SET)) exit(1);
    if (fseek(tx_ptr, 0, SEEK_SET)) exit(1);

    int movect = 0;
    int rx_cnt = 0;
    do {
        rx_cnt = fread(buf, typesz, min(buffsz, movesz - movect), rx_ptr);
        // printf("%s\n", buf);
        fwrite(buf, typesz, rx_cnt, tx_ptr);

        movect += rx_cnt;
    } while (movect < movesz && rx_cnt == buffsz);


    fseek(tx_ptr, 0, SEEK_SET);
    if (ftruncate(fileno(tx_ptr), movect * typesz) != 0)
    {
        fprintf(stderr, "failed to truncate result file\n");
        exit(2);
    }

    fclose(rx_ptr);
    fclose(tx_ptr);
    free(buf);
}
