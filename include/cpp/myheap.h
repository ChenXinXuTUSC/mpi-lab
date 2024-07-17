#ifndef MYHEAP_H
#define MYHEAP_H

#include <initializer_list>
#include <vector>
using namespace std;

template <typename element, typename binary_op>
class heap
{
private:
    vector<element> array;
    binary_op cmp;
    size_t heap_size;

private:
    void sift_up(size_t curr)
    {
        while (curr > 1 && cmp(array[curr >> 1], array[curr]))
        {
            swap(array[curr >> 1], array[curr]); // exchange child and parent
            curr = curr >> 1;
        }
    }
    void sink_dn(size_t curr)
    {
        while ((curr << 1) <= heap_size)
        {
            size_t lson = (curr << 1) + 0;
            size_t rson = (curr << 1) + 1;
            size_t trgt = curr;

            if (lson <= heap_size && cmp(array[trgt], array[lson]))
                trgt = lson;
            if (rson <= heap_size && cmp(array[trgt], array[rson]))
                trgt = rson;
            if (trgt ^ curr)
            {
                swap(array[curr], array[trgt]);
                curr = trgt;
            }
            else
                break;
        }
    }

    void build_heap()
    {
        for (size_t i = heap_size >> 1; i >= 1; --i)
            sink_dn(i);
    }

public:
    heap(binary_op opt)
    {
        array.emplace_back(element()); // empty head
        cmp = opt;
        heap_size = 0;
    }
    heap(binary_op opt, std::initializer_list<element> init_list) : heap(opt)
    {
        heap_size = 0;
        for (const element& ele : init_list)
        {
            array.emplace_back(ele);
            heap_size++;
        }
        build_heap();
    }

    uint size() const
    {
        return heap_size;
    }

    void push(const element& ele)
    {
        heap_size++;
        if (heap_size == array.size())
            array.resize(heap_size * 2);
        array[heap_size] = ele;
        sift_up(heap_size);
    }

    void pop()
    {
        if (!heap_size) return;

        swap(array[1], array[heap_size]);
        heap_size--;
        sink_dn(1);
    }

    element top()
    {
        if (!heap_size) return element();
        return array[1];
    }

    bool empty() const
    {
        return heap_size == 0;
    }
};

#endif
