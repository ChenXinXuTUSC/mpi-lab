#include <common_c.h>

#include <limits.h>
#include <float.h>


bool use_exp_K = false; // 2^10
bool use_exp_M = false; // 2^20
bool use_exp_G = false; // 2^30

unsigned int num = 0;
unsigned int epl = 0;

bool gen_int = false;
bool gen_flt = false;

float gen_random_flt(float seed)
{
    // 随机决定是正数还是负数
    // float sign = (rand() % 2 ? 1.0f : -1.0f);  // 生成 -1 或 1
    
    // 将随机浮点数映射到 [-FLT_MAX, FLT_MAX] 范围
    return 4 * seed * (1.0f - seed);
    // return sign * scale;
}

int gen_random_int()
{
    int sign = (rand() % 2) * 2 - 1;  // 生成 -1 或 1
    return sign * rand();
}

void args_handler(
    const int opt,
    const int optopt,
    const int optind,
    char* optarg
) {
    switch (opt) {
    case 'I':
        gen_int = true;
        break;
    case 'F':
        gen_flt = true;
        break;
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
        break;
    default:
        abort();
    }
}

int main(int argc, char** argv)
{
    parse_args(argc, argv, "hIFKMGN:", &args_handler);
    if ((!gen_int && !gen_flt) || (gen_int && gen_flt))
    {
        fprintf(stderr, "must specified only one mode\n");
        exit(-1);
    }

    printf("N: %d, E: %E\n", num, (double)epl);

    srand((unsigned int)time(NULL));
    float seed = (float)rand() / (float)RAND_MAX;

    const char* exp_str = "";
    if (use_exp_M)  exp_str = "M";
    if (use_exp_K)  exp_str = "K";
    if (use_exp_G)  exp_str = "G";
    const char* typ_str = "";
    if (gen_int) typ_str = "INT";
    if (gen_flt) typ_str = "FLT";

    char filename[128];
    sprintf(filename, "%s%d%s.bin", typ_str, num, exp_str);
    printf("will write to %s\n", filename);

    FILE* fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        perror("failed to open file");
        exit(1);
    }

    size_t typesz;
    if (gen_int) typesz = sizeof(int);
    if (gen_flt) typesz = sizeof(float);

    void* data = malloc(typesz);


    if (use_exp_K) epl = pow(2, 10);
    if (use_exp_M) epl = pow(2, 20);
    if (use_exp_G) epl = pow(2, 30);
    if (!epl)
    {
        fprintf(stderr, "no level specified, abort...");
        exit(1);
    }
    for (size_t i = 0; i < (size_t)(num * epl); ++i)
    {
        if (gen_int)
        {
            int rand_int = gen_random_int();
            memcpy(data, &rand_int, typesz);
        }
        if (gen_flt)
        {
            float rand_flt = gen_random_flt(seed);
            seed = rand_flt;
            memcpy(data, &rand_flt, typesz);
        }

        size_t tx_cnt = fwrite(data, typesz, 1, fp);
        if (tx_cnt < 1)
        {
            fprintf(stderr, "failed to write data, abort\n");
            exit(1);
        }
    }
    fclose(fp);
    printf("finish random data generation\n");

    free(data);
    return 0;
}
