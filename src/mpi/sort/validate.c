#include <common_c.h>

#include <string.h>

char* file_path;
bool use_asc = false;
bool use_dsc = false;
bool print_out = false;
bool force_out = false;

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
        force_out = true;
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

int main(int argc, char** argv)
{
    parse_args(argc, argv, "ADPFf:", &args_handler);
    if (use_asc && use_dsc)
    {
        fprintf(stderr, "must select only 1 validate mode\n");
        exit(1);
    }
    if (file_path == NULL || strlen(file_path) == 0)
    {
        fprintf(stderr, "must specify a valid file path\n");
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
    bool isordered = true;

    if (print_out)
        fprintf(stdout, "%d ", prev);
    while (fread(&curr, sizeof(int), 1, fp) == 1)
    {
        if (print_out)
            fprintf(stdout, "%d ", curr);
        cnt++;
        if ((use_asc && !(prev <= curr)) || (use_dsc && !(prev >= curr)))
        {
            fprintf(stderr, "sequential order check failed at %d with %d and %d\n", cnt, prev, curr);
            if (force_out)
                break;
            isordered = false;
        }

        prev = curr;
    }

    if (isordered)
        printf("\nsequential check passed with %d count\n", cnt);
    else
        printf("\nsequential check passed with %d count\n", cnt);
    
    fclose(fp);
    return 0;
}
