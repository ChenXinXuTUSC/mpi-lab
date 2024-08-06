#include <common_c.h>


char* file_path;
int offset;
int movesz;
int buffsz;
int typesz;

void parse_args(int argc, char** argv);

void c_truncate(const char* file_path, const int offset, const int movesz, const int buffsz, const int typesz);

int min(int a, int b)
{
    if (a < b) return a;
    return b;
}

int main(int argc, char** argv)
{
    parse_args(argc, argv);

    c_truncate(file_path, offset, movesz, buffsz, typesz);

    return 0;
}

void c_truncate(const char* file_path, const int offset, const int movesz, const int buffsz, const int typesz)
{
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

void parse_args(int argc, char** argv)
{
    extern char* optarg;
    extern int   optopt;
    extern int   optind;

    int opt;
    while ((opt = getopt(argc, argv, "f:b:o:n:t:")) != -1)
    {
        switch (opt)
        {
        case 'f':
            file_path = optarg;
            break;
        
        case 'b':
            buffsz = atoi(optarg);
            break;

        case 'o':
            offset = atoi(optarg);
            break;
        
        case 'n':
            movesz = atoi(optarg);
            break;
        
        case 't':
            typesz = atoi(optarg);
            break;
        
        case '?':
            if (optopt == 'o')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            break;
        default:
            abort();
        }
    }
}
