#include <common_cpp.h>
#include <myheap.h>

#include <functional>

template <typename T>
bool read_ele_vec(
    const std::string& filepath,
    const uint ele_need,
    const uint offset,
    std::vector<T>& data_vec
)
{
    std::ifstream finput(filepath, std::ios::binary);
    if (!finput.is_open())
    {
        fprintf(stderr, "failed to open data bin %s\n", filepath.c_str());
        return false;
    }
    finput.seekg(offset * sizeof(T), std::ios::beg);
    if (finput.fail())
    {
        fprintf(stderr, "failed to seek to offset %u\n", offset);
        return false;
    }
    // make sure data vector has enough space

    data_vec.resize(ele_need);
    finput.read(reinterpret_cast<char*>(data_vec.data()), ele_need * sizeof(T));
    uint ele_read = finput.gcount() / sizeof(T);
    if (ele_read < ele_need)
        fprintf(stderr, "number of data %d read less than specified %d\n", ele_read, ele_need);
    data_vec.resize(ele_read);
    finput.close();

    return ele_read == ele_need;
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
    bool toend = false;
    do {
        toend = read_ele_vec(filepath, 1024, 1024 * ksegheap.size(), vec_holder);
        if (vec_holder.size())
        {
            ksegheap.push(vec_holder); // has valid data length
            cout << "seg length: " << vec_holder.size() << " total: " << ksegheap.size() << " first: " << vec_holder.front() << endl;
        }
    } while (toend);


    return 0;
}
