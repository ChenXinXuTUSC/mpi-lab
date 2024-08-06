#include <common_c.h>

char* file_path;
int start;
int num;

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
    
    case 's':
        start = atoi(optarg);
        break;
    
    case 'n':
        num = atoi(optarg);
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
    parse_args(argc, argv, "f:s:n:", &args_handler);

    FILE* fp = fopen(file_path, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "failed to open file %s\n", file_path);
        exit(1);
    }

    for (int i = start; i < start + num; ++i)
    {
        fwrite(&i, 1, sizeof(int), fp);
    }
    fclose(fp);
    return 0;
}
