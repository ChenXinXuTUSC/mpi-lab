#include <common_c.h>

char* file_path;
int start;
int num;

void parse_args(int argc, char** argv);

int main(int argc, char** argv)
{
    parse_args(argc, argv);

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

void parse_args(int argc, char** argv)
{
    extern char* optarg;
    extern int   optopt;
    extern int   optind;

    int opt;
    while ((opt = getopt(argc, argv, "f:s:n:")) != -1)
    {
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
}
