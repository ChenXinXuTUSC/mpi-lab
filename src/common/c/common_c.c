#include <common_c.h>

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

