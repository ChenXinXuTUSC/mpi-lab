#include <common_c.h>

#include <string.h>

#ifdef USE_INT
    typedef int dtype;
    const char* dtype_printfstr = "%d ";
#endif

#ifdef USE_FLT
    typedef float dtype;
    const char* dtype_printfstr = "%f ";
#endif

char* file_path;
bool use_asc = false;
bool use_dsc = false;
bool print_out = false;

void args_handler(
    const int opt,
    const int optopt,
    const int optind,
    char* optarg
) {
    switch(opt)
    {
    case 'A':
        use_asc = true;
        break;
    
    case 'D':
        use_dsc = true;
        break;

    case 'P':
        print_out = true;
        break;
    
    case 'f':
        file_path = optarg;
        break;

    case '?':
        if (isprint(optopt))
            fprintf(stderr, "Unknown option `-%c'.\n", optopt);
        else
            fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
        break;
    default:
        abort();
    }
}

int main(int argc, char** argv)
{
    parse_args(argc, argv, "ADPFIf:", &args_handler);
    if ((use_asc && use_dsc) || (!use_asc && !use_dsc))
    {
        printf("must select only 1 validate mode\n");
        exit(-1);
    }
    if (file_path == NULL || strlen(file_path) == 0)
    {
        printf("must specify a valid file path\n");
        exit(-1);
    }

    FILE* fp = fopen(file_path, "rb");
    if (fp == NULL)
    {
        printf("failed to open %s\n", file_path);
        exit(1);
    }


    dtype prev;
    dtype curr;

    if (fread(&prev, sizeof(dtype), 1, fp) < 1)
    {
        printf("empty data bin file\n");
        exit(1);
    }

    int cnt = 1;
    bool isordered = true;

    if (print_out)
        printf(dtype_printfstr, prev);
    while (fread(&curr, sizeof(dtype), 1, fp) == 1)
    {
        if (print_out)
            printf(dtype_printfstr, curr);
        cnt++;
        if ((use_asc && prev > curr) || (use_dsc && prev < curr))
        {
            printf("sequential order check failed at %d with ", cnt);
            printf(dtype_printfstr, curr);
            printf("\n");
            isordered = false;
        }

        prev = curr;
    }

    if (isordered)
        printf("\nsequential check passed with %d count\n", cnt);
    else
        printf("\nsequential check failed with %d count\n", cnt);
    
    fclose(fp);
    return 0;
}
