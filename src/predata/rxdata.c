#include <common_c.h>

#ifdef USE_INT
    typedef int dtype;
#endif

#ifdef USE_FLT
    typedef float dtype;
#endif

char* file_path;
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
        file_path = optarg;
        break;

    case 'P':
        print_out = true;
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
    parse_args(argc, argv, "Pf:", &args_handler);

    FILE* fp = fopen(file_path, "rb");
    if (fp == NULL)
    {
        perror("error open data bin");
        exit(1);
    }

    dtype data;


    int cnt = 0;
    while (fread(&data, sizeof(dtype), 1, fp) == 1)
    {
        cnt++;
        if (print_out)
        {
            #ifdef USE_INT
                printf("%d ", data);
            #endif
            #ifdef USE_FLT
                printf("%f ", data);
            #endif
        }
    }

    if (feof(fp))
        printf("\nfinish reading %s, total %d\n", file_path, cnt);
    else
        printf("error reading data bin\n");

    fclose(fp);
    return 0;
}
