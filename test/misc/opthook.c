#include <common_c.h>

int a;
int b;
bool verbose;

void parse_args(
    int argc, char** argv,
    const char* shortopts,
    void(*args_handler)(
        const int opt,
        const int optopt,
        const int optind,
        char* optarg
    )
);

void args_handler(
    const int opt,
    const int optopt,
    const int optind,
    char* optarg
) {
    switch (opt)
    {
    case 'a':
        a = atoi(optarg);
        break;
    
    case 'b':
        b = atoi(optarg);
        break;
    
    case 'v':
        verbose = true;
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
    parse_args(argc, argv, "a:b:v", &args_handler);

    if (verbose)
        fprintf(stdout, "data a: %d, data b: %d\n", a, b);
    else
        fprintf(stdout, "%d %d\n", a, b);
    return 0;
}

void parse_args(
    int argc, char** argv,
    const char* shortopts,
    void(*args_handler)(
        const int opt,
        const int optopt,
        const int optind,
        char* optarg
    )
)
{
    extern char* optarg;
    extern int   optopt;
    extern int   optind;

    int opt;
    while ((opt = getopt(argc, argv, shortopts)) != -1)
    {
        args_handler(opt, optopt, optind, optarg);
    }
}

