#include <common_c.h>

#include <limits.h>
#include <float.h>


bool use_exp_K = false; // 2^10
bool use_exp_M = false; // 2^20
bool use_exp_G = false; // 2^30

unsigned int num = 0;
unsigned int epl = 0;

#ifdef USE_INT
    typedef int dtype;
    const char* dtype_str = "INT";

    int random_data(int seed)
    {
        int sign = (rand() % 2) * 2 - 1;  // 生成 -1 或 1
        return sign * rand();
    }
#endif

#ifdef USE_FLT
    typedef float dtype;
    const char* dtype_str = "FLT";

    float random_data(float seed)
    {
        return 4 * seed * (1.0f - seed);
    }
#endif

void args_handler(
    const int opt,
    const int optopt,
    const int optind,
    char* optarg
) {
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
        printf("gendata [-MKG] -N <num>");
        break;
    case '?':
        if (isprint(optopt))
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
    parse_args(argc, argv, "hKMGN:", &args_handler);

    srand((unsigned int)time(NULL));

    const char* exp_str = "";
    if (use_exp_M)  exp_str = "M";
    if (use_exp_K)  exp_str = "K";
    if (use_exp_G)  exp_str = "G";

    if (use_exp_K) epl = pow(2, 10);
    if (use_exp_M) epl = pow(2, 20);
    if (use_exp_G) epl = pow(2, 30);
    if (!epl)
    {
        printf("no exponent specified, abort...");
        exit(-1);
    }

    char filename[128];
    sprintf(filename, "%s%d%s.bin", dtype_str, num, exp_str);
    printf("will write to %s\n", filename);

    FILE* fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        perror("failed to open file");
        exit(1);
    }

    dtype data = (dtype)rand() / (dtype)RAND_MAX;
    for (size_t i = 0; i < (size_t)(num * epl); ++i)
    {
        data = random_data(data);
        size_t tx_cnt = fwrite(&data, sizeof(dtype), 1, fp);
        if (tx_cnt < 1)
        {
            printf("failed to write data, abort\n");
            exit(1);
        }
    }

    fclose(fp);
    return 0;
}
