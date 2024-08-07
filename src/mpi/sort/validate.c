#include <common_c.h>

#include <string.h>

char* file_path;
bool use_asc = false;
bool use_dsc = false;
bool print_out = false;
bool is_int = false;
bool is_flt = false;

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

    case 'F':
        is_flt = true;
        break;
    
    case 'I':
        is_int = true;
    
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

void print_data(void* data, bool is_int, bool is_flt) {
    if ((is_int && is_flt) || (!is_int && !is_flt) || data == NULL)
    {
        printf("must select only 1 data type\n");
        return;
    }
    if (is_int) printf("%d ", *((int*)data));
    if (is_flt) printf("%f ", *((float*)data));
}

bool cmpt(void* d1, void* d2, bool is_int, bool is_flot, bool asc)
{
    if ((is_int && is_flt) || (!is_int && !is_flt) || d1 == NULL || d2 == NULL)
    {
        printf("must select only 1 data type\n");
        return false;
    }
    if (is_int)
    {
        int di1 = *((int*)d1);
        int di2 = *((int*)d2);
        if (asc) return di1 <= di2;
        else return di1 >= di2;
    }
    if (is_flt)
    {
        float df1 = *((float*)d1);
        float df2 = *((float*)d2);
        if (asc) return df1 <= df2;
        else return df1 >= df2;
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
    if ((is_int && is_flt) || (!is_int && !is_flt))
    {
        printf("must select only 1 data type\n");
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

    size_t typesz;
    if (is_int) typesz = sizeof(int);
    if (is_flt) typesz = sizeof(float);

    void* prev = malloc(typesz);
    void* curr = malloc(typesz);

    if (fread(prev, typesz, 1, fp) < 1)
    {
        printf("empty data bin file\n");
        exit(1);
    }

    int cnt = 1;
    bool isordered = true;

    if (print_out)
        print_data(prev, is_int, is_flt);
    while (fread(curr, typesz, 1, fp) == 1)
    {
        if (print_out)
            print_data(curr, is_int, is_flt);
        cnt++;
        if ((use_asc && !cmpt(prev, curr, is_int, is_flt, true)) || (use_dsc && !cmpt(prev, curr, is_int, is_flt, false)))
        {
            printf("sequential order check failed at %d with ", cnt);
            print_data(prev, is_int, is_flt);
            print_data(curr, is_int, is_flt);
            printf("\n");
            isordered = false;
        }

        if (is_int) *((int*)prev) = *((int*)curr);
        if (is_flt) *((float*)prev) = *((float*)curr);
    }

    if (isordered)
        printf("\nsequential check passed with %d count\n", cnt);
    else
        printf("\nsequential check passed with %d count\n", cnt);
    
    fclose(fp);
    free(prev);
    free(curr);
    return 0;
}
