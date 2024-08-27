#include <common_c.h>

#ifdef USE_INT
    typedef int dtype;
#endif

#ifdef USE_FLT
    typedef float dtype;
#endif

char* input_path;
char* output_path;
bool print_out;

void args_handler(
    const int opt,
    const int optopt,
    const int optind,
    char* optarg
) {
    switch (opt)
    {
    case 'f':
        input_path = optarg;
        break;

    case 'o':
        output_path = optarg;
        break;
    
    case '?':
        if (optopt == 'o')
            printf("Option -%c requires an argument.\n", optopt);
        else if (isprint(optopt))
            printf("Unknown option `-%c'.\n", optopt);
        else
            printf("Unknown option character `\\x%x'.\n", optopt);
        break;
    default:
        abort();
    }
}

int main(int argc, char** argv)
{
    parse_args(argc, argv, "f:o:", &args_handler);

    FILE* finp = fopen(input_path, "rb");
    if (finp == NULL)
    {
        perror("error open input data bin");
        exit(1);
    }

    FILE* foup = fopen(output_path, "wb");
    if (foup == NULL)
    {
        perror("error open output data bin");
        exit(1);
    }

    dtype data;

    long cnt_dd = 0;
    fseek(finp, 0, SEEK_END);
    cnt_dd = ftell(finp) / sizeof(dtype);

    printf("will reverse %ld data\n", cnt_dd);

    long cnt_tl = 1;
    long cnt_rx = 0;
    long cnt_tx = 0;
    do {
        fseek(finp, -sizeof(dtype) * cnt_tl, SEEK_END);
        cnt_rx = fread(&data, sizeof(dtype), 1, finp);
        cnt_tx = fwrite(&data, sizeof(dtype), 1, foup);
        if (cnt_rx == 0 || cnt_tx == 0)
        {
            printf("error rx/tx, exit...\n");
            exit(-1);
        }
        cnt_tl++;
    } while (cnt_tl <= cnt_dd);

    fclose(finp);
    fclose(foup);
    return 0;
}
