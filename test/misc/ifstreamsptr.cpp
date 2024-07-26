#include <common_cpp.h>
#include <memory>

int main(int argc, char** argv)
{
    using std::cout;
    using std::cerr;
    using std::endl;

    std::shared_ptr<std::ifstream> finput = std::make_shared<std::ifstream>("data/temp/0.bin", std::ifstream::in | std::ifstream::binary);
    if (!finput->is_open()) {
        fprintf(stderr, "failed to open data bin\n");
        exit(1);
    }

    int tmp;
    do {
        finput->read(reinterpret_cast<char*>(&tmp), sizeof(int));
        cout << tmp << ' ';
    } while (finput->gcount() != 0);

    return 0;
}
