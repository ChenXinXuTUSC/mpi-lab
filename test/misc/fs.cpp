#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>

using namespace std;
namespace fs = filesystem;

int main(int argc, char** argv)
{
    fs::path base = "data/output";
    fs::path totl = base / "final.bin";

    std::ifstream abc(base, ifstream::binary);
    if (abc.is_open())
        cout << "file successfully opened" << endl;
    abc.close();

    cout << totl << endl;

    std::vector<std::string> def {
        base.c_str(),
        totl.c_str()
    };

    return 0;
}
