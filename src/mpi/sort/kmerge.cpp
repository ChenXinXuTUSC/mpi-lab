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
    std::ifstream finput(filepath, std::ifstream::in | std::ifstream::binary);
    if (!finput.is_open())
    {
        fprintf(stderr, "failed to open data bin %s\n", filepath.c_str());
        return false;
    }
    finput.seekg(offset * sizeof(T), std::ifstream::beg);
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
    
    std::function<bool(const queue<int>&, const queue<int>&)> cmp = [](
        const queue<int>& q1,
        const queue<int>& q2
    ) {
        return q1.front() > q2.front(); // ascend heap, not descend heap
    };
    heap<std::queue<int>, decltype(cmp)> ksegheap(cmp);

    // read in data and distribute to segments
    std::vector<int> vec_holder;
    bool toend = false;
    do {
        toend = read_ele_vec(filepath, 1024, 1024 * ksegheap.size(), vec_holder);
        if (vec_holder.size())
        {
            sort(vec_holder.begin(), vec_holder.end());
            queue<int> que_holder;
            for (const int& num : vec_holder)
                que_holder.push(num);
            ksegheap.push(que_holder); // has valid data length
            cout << "seg length: " << que_holder.size() << " total: " << ksegheap.size() << " first: " << que_holder.front() << endl;
        }
    } while (toend);

    // sort sub-segments into a big one
    std::ofstream foutput("data/sorted.bin", std::ios::out | std::ios::binary);
    while (!ksegheap.empty())
    {
        int num = ksegheap.top().front();
        foutput.write(reinterpret_cast<char*>(&num), sizeof(int));
        auto que = ksegheap.top();
        ksegheap.pop();

        que.pop();
        if (!que.empty())
            ksegheap.push(que);
    }
    foutput.close();
    return 0;
}
