#include <common_c.h>

#include <math.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>

#include <getopt.h>

bool use_exp_K = false; // 2^10
bool use_exp_M = false; // 2^20
bool use_exp_G = false; // 2^30

unsigned int num = 0;
unsigned int epl = 0;

void parse_opt(int argc, char** argv)
{
    extern char* optarg;
    extern int   optopt;
    extern int   optind;

    int opt;
    while ((opt = getopt(argc, argv, "hKMGN:")) != -1) {
        switch (opt) {
        case 'K':
            use_exp_K = true;
            break;
        case 'M':
            use_exp_M = true;
            break;
        case 'G':
            use_exp_G = true;
            break;
        case 'N':
            num = atoi(optarg);
            break;
        case 'h':
            fprintf(stdout, "gendata [-MKG] -N <num>");
            break;
        case '?':
            if (optopt == 'o')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
        default:
            abort();
        }
    }

    if (use_exp_K) epl = pow(2, 10);
    if (use_exp_M) epl = pow(2, 20);
    if (use_exp_G) epl = pow(2, 30);
    if (!epl)
    {
        fprintf(stderr, "no level specified, abort...");
        exit(1);
    }

    // Remaining arguments after specififed options
    for (int i = optind; i < argc; i++) {
        printf("Non-option argument: %s\n", argv[i]);
    }
}

int main(int argc, char** argv)
{
    srand((unsigned int)time(NULL));

    parse_opt(argc, argv);
    printf("N: %d, E: %E\n", num, (double)epl);

    const char* exp_str = "";
    if (use_exp_M)  exp_str = "M";
    if (use_exp_K)  exp_str = "K";
    if (use_exp_G)  exp_str = "G";
    char filename[128];
    sprintf(filename, "%d%s.bin", num, exp_str);
    printf("will write to %s\n", filename);

    FILE* fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        perror("failed to open file");
        exit(1);
    }
    for (size_t i = 0; i < (size_t)(num * epl); ++i)
    {
        int rand_int = rand() * (rand() % 2 ? 1 : -1);
        size_t bytes_txed = fwrite(&rand_int, sizeof(int), 1, fp);
        if (bytes_txed < 1)
        {
            fprintf(stderr, "failed to write data, abort\n");
            exit(1);
        }
    }
    fclose(fp);
    printf("finish random data generation\n");
    return 0;
}
