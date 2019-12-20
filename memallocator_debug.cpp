#include <iostream>
#include "memallocator.h"

using namespace std;

int main()
{
    MemoryAllocator allocator;
    allocator.init();

    int* pi = (int*)allocator.alloc(sizeof(int));
    double* pd = (double*)allocator.alloc(sizeof(double));
    int* pa = (int*)allocator.alloc(10 * sizeof(int));
    int* pl[10];
    for (size_t i = 0; i < 10; ++i)
    {
        pl[i] = (int*)allocator.alloc(100 * sizeof(int));
    }

    int* bpi = (int*)allocator.alloc(1000 * sizeof(int));
    double* bpd = (double*)allocator.alloc(1000 * sizeof(double));
    double* bpl = (double*)allocator.alloc(1000 * sizeof(long));
    int* bpa = (int*)allocator.alloc(1000000 * sizeof(int));

#ifndef NDEBUG
    std::cout << "Before user freeing:\n";
    std::cout << "Overall memory statistics:\n";
    allocator.dumpStat();
    std::cout << "\nAllocated memory statistics:\n";
    allocator.dumpBlocks();
    std::cout << std::endl;
#endif

    allocator.free(pa);
    allocator.free(pd);
    allocator.free(pi);
    for (auto p : pl)
    {
        allocator.free(p);
    }

    allocator.free(bpa);
    allocator.free(bpl);
    allocator.free(bpi);
    allocator.free(bpd);

#ifndef NDEBUG
    std::cout << "After user freeing:\n";
    std::cout << "Overall memory statistics:\n";
    allocator.dumpStat();
    std::cout << "\nAllocated memory statistics:\n";
    allocator.dumpBlocks();
    std::cout << std::endl;
#endif

    std::cout << "Checking detecting leaks (should report 2 leaks):\n";
    allocator.alloc(128);
    allocator.alloc(1024);
    return 0;
}