#include <common_cpp.h>
#include <filesystem>

#include <getopt.h>
#include <ctype.h>

void parse_args(int argc, char** argv);

char* file_path;
int intercept_size;

int main(int argc, char** argv)
{
    parse_args(argc, argv);

    if (file_path == NULL || intercept_size <= 0)
    {
        fprintf(stderr, "invalid arguments: %s, %d\n", file_path, intercept_size);
        exit(-1);
    }


    std::ifstream finput(file_path, std::ifstream::in | std::ifstream::binary);
    if (!finput.is_open())
    {
        fprintf(stderr, "failed to open %s\n", file_path);
        exit(1);
    }

    int file_size = std::filesystem::file_size(file_path);
    if (file_size < intercept_size * 2)
    {
        fprintf(stderr, "file size %d should at least be 2x intercept size %d\n", file_size, intercept_size);
        exit(1);
    }

    std::vector<int> rx_buf(intercept_size);

    // front segment
    {
        std::ofstream foutput("front.bin", std::ofstream::out | std::ofstream::binary);
        if (!foutput.is_open())
        {
            fprintf(stderr, "failed to open save file front.bin\n");
            exit(2);
        }
        finput.read(reinterpret_cast<char*>(rx_buf.data()), sizeof(int) * intercept_size);
        foutput.write(reinterpret_cast<char*>(rx_buf.data()), sizeof(int) * intercept_size);
        foutput.close();
    }
    // rear segment
    {
        std::ofstream foutput("back.bin", std::ofstream::out | std::ofstream::binary);
        if (!foutput.is_open())
        {
            fprintf(stderr, "failed to open save file back.bin\n");
            exit(2);
        }
        finput.seekg(file_size - intercept_size, std::ios::beg);
        finput.read(reinterpret_cast<char*>(rx_buf.data()), sizeof(int) * intercept_size);
        foutput.write(reinterpret_cast<char*>(rx_buf.data()), sizeof(int) * intercept_size);
        foutput.close();
    }


    return 0;
}

void parse_args(int argc, char** argv)
{
    extern char* optarg;
    extern int   optopt;
    extern int   optind;

    int opt;
    while ((opt = getopt(argc, argv, "f:n:")) != -1)
    {
        switch (opt)
        {
        case 'f':
            file_path = optarg;
            break;
        
        case 'n':
            intercept_size = atoi(optarg);
            break;
        
        case '?':
            if (optopt == 'h')
                fprintf(stderr, "%d no help info\n", optopt);
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
