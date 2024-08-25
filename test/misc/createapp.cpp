#include <common_cpp.h>

int main(int argc, char** argv)
{
    std::ofstream foutput("hellout.txt", std::ofstream::app);
    if (!foutput.is_open())
    {
        std::cerr << "error opening hellout.txt" << std::endl;
        exit(-1);
    }

    foutput << "hello world\n";

    return 0;
}
