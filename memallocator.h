#ifndef MEMALLOCATOR_H_INCLUDED
#define MEMALLOCATOR_H_INCLUDED
#include <cstdint>

class FSAllocator;
class CoaleseAllocator;

class MemoryAllocator
{
    enum { ALIGN = 8 };
    enum { BYTES_ON_SWITCH_ALLOCATOR = 512 };
public:
	MemoryAllocator();
	virtual ~MemoryAllocator();

    MemoryAllocator(const MemoryAllocator&) = delete;
    MemoryAllocator& operator=(const MemoryAllocator&) = delete;

    virtual void init();
    virtual void destroy();

    virtual void* alloc(size_t);
    virtual void free(void*);

#ifndef NDEBUG
    virtual void dumpStat() const;
    virtual void dumpBlocks() const;
#endif

private:
	FSAllocator* fs_al = nullptr;
	CoaleseAllocator* co_al = nullptr;
};

#endif // !MEMALLOCATOR_H_INCLUDED
