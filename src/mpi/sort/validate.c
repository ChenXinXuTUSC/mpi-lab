#include <common_c.h>

#include <getopt.h>
#include <ctype.h>

char* file_path;
bool use_asc = false;
bool use_dsc = false;

void parse_args(int argc, char** argv)
{
    extern char* optarg;
    extern int   optopt;
    extern int   optind;

    int opt;
    while ((opt = getopt(argc, argv, "ADf:")) != -1)
    {
        switch(opt)
        {
        case 'A':
            use_asc = true;
            break;
        
        case 'D':
            use_dsc = true;
            break;
        
        case 'f':
            file_path = optarg;
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

int main(int argc, char** argv)
{
    parse_args(argc, argv);
    if (use_asc && use_dsc)
    {
        fprintf(stderr, "must select only 1 validate mode\n");
        exit(1);
    }

    FILE* fp = fopen(file_path, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "failed to open %s\n", file_path);
        exit(1);
    }

    int prev, curr;
    if (fread(&prev, sizeof(int), 1, fp) < 1)
    {
        fprintf(stderr, "empty data bin file\n");
        exit(1);
    }
    int cnt = 1;
    while (fread(&curr, sizeof(int), 1, fp) == 1)
    {
        cnt++;
        if ((use_asc && !(prev <= curr)) || (use_dsc && !(prev >= curr)))
        {
            fprintf(stderr, "sequential order check failed at %d with %d and %d\n", cnt, prev, curr);
            exit(1);
        }
    }

    printf("sequential check passed with %d count\n", cnt);
    fclose(fp);
    return 0;
}
