#include <common_c.h>

bool is_int;
bool is_flt;
char* file_path;

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
    
    case 'I':
        is_int = true;
        break;
    
    case 'F':
        is_flt = true;
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

int main(int argc, char** argv)
{
    parse_args(argc, argv, "IFf:", &args_handler);

    if ((is_int && is_flt) || (!is_int && !is_flt))
    {
        fprintf(stderr, "must specify only one type\n");
        exit(-1);
    }

    FILE* fp = fopen(file_path, "rb");
    if (fp == NULL)
    {
        perror("error open data bin");
        exit(1);
    }

    int typesz;
    if (is_int) typesz = sizeof(int);
    if (is_flt) typesz = sizeof(float);
    void* data = malloc(typesz);


    int cnt = 0;
    while (fread(data, typesz, 1, fp) == 1)
    {
        cnt++;
        if (is_int) printf("%d ", *((int*)(data)));
        if (is_flt) printf("%f ", *((float*)(data)));
    }

    if (feof(fp))
        printf("\nfinish reading data bin, total %d\n", cnt);
    else
        fprintf(stderr, "error reading data bin\n");

    fclose(fp);
    free(data);
    return 0;
}
