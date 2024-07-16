#include <common_cpp.h>
#include <myheap.h>

#include <functional>

template <typename T>
bool read_ele_vec(
    const std::string& filepath,
    const uint read_amt,
    std::vector<T>& data_vec
)
{
    std::ifstream istrm(filepath, std::ios::binary);
    if (istrm.is_open())
    {
        fprintf(stderr, "failed to open data bin %s\n", filepath.c_str());
        return false;
    }

    int read_cnt = 0;
    T data_holder;
    while (read_cnt < read_amt)
    {
        istrm.read(reinterpret_cast<char*>(&data_holder), sizeof(T));
        data_vec.emplace_back(data_holder);
        read_cnt++;
    }
    return read_cnt == read_amt;
}

int main(int argc, char** argv)
{
    const std::string filepath = "data/10K.bin";
    
    std::function<bool(const vector<int>&, const vector<int>&)> cmp = [](
        const vector<int>& v1,
        const vector<int>& v2
    ) {
        return v1.front() < v2.front();
    };
    heap<std::vector<int>, decltype(cmp)> ksegheap(cmp);

    std::vector<int> vec_holder;
    while (read_ele_vec(filepath, 1024, vec_holder))
        ksegheap.push(vec_holder);

    return 0;
}
