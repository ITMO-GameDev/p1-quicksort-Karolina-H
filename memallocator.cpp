#include <cassert>
#include "memallocator.h"

#ifndef NDEBUG
#include <iostream>
#include <iomanip>
#endif

class FSAllocator
{
    enum { ALIGN = 8 };
    enum { MIN_BYTES = 16 };
    enum { MAX_BYTES = 512 };
    enum { BUCKETS_COUNT = 6 };
    enum { CHUNK_SIZE = 4096 - 4 * sizeof(void*) };

public:
    FSAllocator() = default;

    ~FSAllocator()
    {
        assert(state != State::Destroyed);
        destroy();
    }

    FSAllocator(const FSAllocator&) = delete;
    FSAllocator& operator=(const FSAllocator&) = delete;

    void init();
    void destroy();

    void* alloc(size_t);
    void free(void*);

#ifndef NDEBUG
    void dumpStat() const;
    void dumpBlocks() const;
#endif

private:
    using BucketMap = unsigned short;

    struct BlockList
    {
        void* chunk;
        BlockList* next;
    };

    union FreeList
    {
        FreeList* next;

        struct Header
        {
            uint64_t size;

            void init(uint64_t sz)
            {
                size = 0xdead;
                size <<= 48;
                size |= sz;
            }

            uint64_t getSize() const { return size & 0x0000'ffff'ffff'ffff; }

            void release() { size = 0; }

            bool isValid() const { return ((size >> 48) & 0xffff) == 0xdead; }
        } header;
    };

    struct Bucket
    {
        FreeList* first = nullptr;
        BlockList* address = nullptr;
    };

    BucketMap* bucket_map = nullptr;
    Bucket* buckets = nullptr;

#ifndef NDEBUG
    enum class State
    {
        NotInitialized,
        Initialized,
        Destroyed
    };
    State state = State::NotInitialized;
#endif
};

void FSAllocator::init()
{
    assert(state != State::Initialized);

    bucket_map = ::new BucketMap[MAX_BYTES + 1];
    BucketMap* pbucket_type = bucket_map;
    BucketMap bucket_type_max = MIN_BYTES;
    BucketMap bucket_type = 0;
    for (BucketMap b = 0; b <= MAX_BYTES; ++b)
    {
        if (b > bucket_type_max)
        {
            bucket_type_max <<= 1;
            ++bucket_type;
        }
        *pbucket_type++ = bucket_type;
    }
    buckets = ::new Bucket[BUCKETS_COUNT];

#ifndef NDEBUG
    state = State::Initialized;
#endif
}

void FSAllocator::destroy()
{
    assert(state != State::Destroyed);

    if (buckets)
    {
        for (BucketMap i = 0; i < BUCKETS_COUNT; ++i)
        {
#ifndef NDEBUG
            const BucketMap bucket_type_size = (MIN_BYTES << i) + ALIGN;
            const size_t bucket_type_count = (CHUNK_SIZE - sizeof(BlockList)) / bucket_type_size;
#endif
            Bucket& bucket = buckets[i];
            while (bucket.address)
            {
                BlockList* block = bucket.address;
#ifndef NDEBUG
                char* ptr = static_cast<char*>(block->chunk) + sizeof(BlockList);
                for (size_t j = 0; j < bucket_type_count; ++j)
                {
                    const FreeList* record = reinterpret_cast<const FreeList*>(ptr);
                    if (record->header.isValid())
                    {
                        std::cout << "LEAK: at Addr=" << (void*)(reinterpret_cast<const char*>(record) + ALIGN)
                            << " FSABlkSz=" << (MIN_BYTES << i)
                            << " UserReqSz=" << record->header.getSize() << "\n";
                    }
                    ptr += bucket_type_size;
                }
#endif
                bucket.address = block->next;
                ::operator delete(block->chunk);
            }
        }
        ::delete[] buckets;
        ::delete[] bucket_map;
    }
#ifndef NDEBUG
    state = State::Destroyed;
#endif
}

void* FSAllocator::alloc(size_t nbytes)
{
    assert(state == State::Initialized);

    char* result = nullptr;
    if (nbytes > MAX_BYTES)
    {
        result = static_cast<char*>(::operator new(nbytes + ALIGN));
        FreeList* record = reinterpret_cast<FreeList*>(result);
        record->header.init(nbytes);
        result += ALIGN;
    }
    else if (nbytes > 0)
    {
        FreeList* record;
        BucketMap which = bucket_map[nbytes];
        Bucket& bucket = buckets[which];
        if (!bucket.first)
        {
            const BucketMap bucket_type_size = (MIN_BYTES << which) + ALIGN;
            size_t bucket_type_count = (CHUNK_SIZE - sizeof(BlockList)) / bucket_type_size;

            void* chunk = ::operator new(CHUNK_SIZE);
            BlockList* block = static_cast<BlockList*>(chunk);
            block->chunk = chunk;
            block->next = bucket.address;
            bucket.address = block;

            char* ptr = static_cast<char*>(chunk) + sizeof(BlockList);
            record = reinterpret_cast<FreeList*>(ptr);
            bucket.first = record;
            while (--bucket_type_count > 0)
            {
                ptr += bucket_type_size;
                record->next = reinterpret_cast<FreeList*>(ptr);
                record = record->next;
            }
            record->next = 0;
        }
        record = bucket.first;
        bucket.first = record->next;

        record->header.init(nbytes);
        result = reinterpret_cast<char*>(record) + ALIGN;
    }
    return static_cast<void*>(result);
}

void FSAllocator::free(void* ptr)
{
    assert(state == State::Initialized);

    if (ptr)
    {
        char* p = static_cast<char*>(ptr) - ALIGN;
        FreeList* record = reinterpret_cast<FreeList*>(p);

        assert(record->header.isValid());

        auto nbytes = record->header.getSize();
        record->header.release();
        
        if (nbytes > MAX_BYTES)
        {
            ::operator delete(p);
        }
        else
        {
            BucketMap which = bucket_map[nbytes];
            Bucket& bucket = buckets[which];
            FreeList* block = reinterpret_cast<FreeList*>(p);

            block->next = bucket.first;
            bucket.first = block;
        }
    }
}


class CoaleseAllocator
{
    enum { ALIGN = 8 };
    enum { MIN_BYTES = 512 };
    enum { MAX_BYTES = 10 * 1024 * 1024 };
    enum { CHUNK_SIZE = 1024 * 1024 - 4 * sizeof(void*) };

public:
    CoaleseAllocator() = default;

    ~CoaleseAllocator()
    {
        assert(state != State::Destroyed);
        destroy();
    }

    CoaleseAllocator(const CoaleseAllocator&) = delete;
    CoaleseAllocator& operator=(const CoaleseAllocator&) = delete;

    void init()
    {
        assert(state != State::Initialized);

        reserveBlock();

#ifndef NDEBUG
        state = State::Initialized;
#endif
    }

    void destroy();

    void* alloc(size_t nbytes);
    void free(void* ptr);

#ifndef NDEBUG
    void dumpStat() const;
    void dumpBlocks() const;
#endif

private:
    struct Header;
    struct BlockList
    {
        void* chunk;
        BlockList* next;
        Header* first;
        size_t size;

        Header* getFirstRecord() const
        {
            char* ptr = static_cast<char*>(chunk) + sizeof(BlockList);
            return reinterpret_cast<Header*>(ptr);
        }
    };

    struct Header
    {
        Header* next_or_first;
        uint64_t size;

        void init(uint64_t sz)
        {
            size = 0xdead;
            size <<= 48;
            size |= sz;
        }

        uint64_t getSize(bool drop_busy = true) const
        {
            if (drop_busy)
                return size & 0x0000'ffff'ffff'fff8;
            return size & 0x0000'ffff'ffff'ffff;
        }

        void aquire() { size |= 1; }

        void release() { size &= ~1; }

        bool isBusy() const { return size & 1; }

        bool isValid() const { return ((size >> 48) & 0xffff) == 0xdead; }

        Header* nextHeader()
        {
            char* p = reinterpret_cast<char*>(this) + align(sizeof(Header));
            p += getSize();
            return reinterpret_cast<Header*>(p);
        }

        void coalese(Header* rhs)
        {
            size_t coalese_size = (size_t)(getSize() + align(sizeof(Header)) + rhs->getSize());
            init(coalese_size);
            next_or_first = rhs->next_or_first;
        }
    };

    template<typename T, uint8_t Align = ALIGN>
    static T align(T val) { return (val + Align - 1) & ~static_cast<T>(Align - 1); }

    void reserveBlock(size_t nbytes = CHUNK_SIZE);

    BlockList* block_list = nullptr;

#ifndef NDEBUG
    enum class State
    {
        NotInitialized,
        Initialized,
        Destroyed
    };
    State state = State::NotInitialized;
#endif
};

void CoaleseAllocator::destroy()
{
    assert(state != State::Destroyed);

    while (block_list)
    {
        BlockList* block = block_list;
#ifndef NDEBUG
        Header* record = block->getFirstRecord();
        size_t size = align(sizeof(Header));
        while (record && size < block->size)
        {
            size += (size_t)record->getSize();
            if (record->isBusy())
            {
                std::cout << "LEAK: at Addr="
                    << (void*)(reinterpret_cast<char*>(record) + align(sizeof(Header)))
                    << " of Size=" << record->getSize() << '\n';
            }
            record = record->nextHeader();
        }
#endif // !NDEBUG
        block_list = block->next;
        ::operator delete(block->chunk);
    }

#ifndef NDEBUG
    state = State::Destroyed;
#endif
}

void* CoaleseAllocator::alloc(size_t nbytes)
{
    assert(state == State::Initialized);

    char* result = nullptr;
    if (nbytes > MAX_BYTES)
    {
        size_t alloc_size = align(sizeof(Header)) + nbytes;
        result = static_cast<char*>(::operator new(alloc_size));
        Header* record = reinterpret_cast<Header*>(result);
        record->init(nbytes);
        result += align(sizeof(Header));
    }
    else
    {
        BlockList* block = block_list;
        Header* record = nullptr;
        Header* prev_record = nullptr;
        while (block)
        {
            record = block->first;
            while (record && (size_t)record->getSize() < nbytes)
            {
                prev_record = record;
                record = record->next_or_first;
            }
            if (record)
                break;
            block = block->next;
        }
        if (!record)
        {
            size_t block_size = align(sizeof(BlockList)) + align(sizeof(Header)) + nbytes;
            block_size = block_size > CHUNK_SIZE ? block_size : CHUNK_SIZE;
            reserveBlock(block_size);
            block = block_list;
            record = block->getFirstRecord();
            prev_record = nullptr;
        }

        size_t size_avail = (size_t)record->getSize();
        size_t result_size = size_avail;
        Header* next_record = nullptr;
        if (size_avail - nbytes < MIN_BYTES)
        {
            if (prev_record)
                prev_record->next_or_first = record->next_or_first;
            else
                block->first = record->next_or_first;
        }
        else
        {
            result_size = align(nbytes);
            next_record = (Header*)((char*)(record)+align(sizeof(Header)) + result_size);
            next_record->init(size_avail - result_size);
            next_record->next_or_first = record->next_or_first;

            if (prev_record)
                prev_record->next_or_first = next_record;
            else
                block->first = next_record;
        }

        record->next_or_first = block->getFirstRecord();
        record->init(result_size);
        record->aquire();
        result = reinterpret_cast<char*>(record) + align(sizeof(Header));
    }
    return static_cast<void*>(result);
}

void CoaleseAllocator::free(void* ptr)
{
    assert(state == State::Initialized);

    if (ptr)
    {
        char* p = static_cast<char*>(ptr) - align(sizeof(Header));
        Header* record = reinterpret_cast<Header*>(p);

        assert(record->isValid());

        size_t nbytes = (size_t)record->getSize(false);
        if (nbytes > MAX_BYTES)
        {
            ::operator delete(p);
        }
        else
        {
            assert(record->isBusy());

            record->release();

            p = reinterpret_cast<char*>(record->next_or_first) - align(sizeof(BlockList));
            BlockList* block = reinterpret_cast<BlockList*>(p);

            if (!block->first)
            {
                record->next_or_first = block->first;
                block->first = record;
            }
            else if (record < block->first)
            {
                if (record->nextHeader() == block->first)
                    record->coalese(block->first);
                else
                    record->next_or_first = block->first;
                block->first = record;
            }
            else
            {
                Header* prev_record = block->first;
                Header* next_record = prev_record->next_or_first;
                while (next_record && next_record < record)
                {
                    prev_record = next_record;
                    next_record = next_record->next_or_first;
                }

                if (next_record && record->nextHeader() == next_record)
                    record->coalese(next_record);
                else
                    record->next_or_first = next_record;

                if (prev_record->nextHeader() == record)
                    prev_record->coalese(record);
                else
                    prev_record->next_or_first = record;
            }
        }
    }
}
void CoaleseAllocator::reserveBlock(size_t nbytes)
{
    void* chunk = ::operator new(nbytes);
    BlockList* block = static_cast<BlockList*>(chunk);
    block->chunk = chunk;

    char* ptr = static_cast<char*>(chunk) + align(sizeof(BlockList));
    Header* record = reinterpret_cast<Header*>(ptr);
    record->next_or_first = nullptr;
    nbytes -= align(sizeof(BlockList));
    block->size = nbytes;
    nbytes -= align(sizeof(Header));
    record->init(nbytes);
    block->first = record;

    block->next = block_list;
    block_list = block;
}

#ifndef NDEBUG

void FSAllocator::dumpStat() const
{
    assert(state == State::Initialized);

    std::cout << "\n==========================================================\n";

    std::cout << "FSA:\n";
    size_t total_reserved = 0;
    size_t total_used = 0;
    size_t total_mem = 0;
    for (BucketMap i = 0; i < BUCKETS_COUNT; ++i)
    {
        const BucketMap fsa_size = MIN_BYTES << i;
        const BucketMap bucket_type_size = fsa_size + ALIGN;
        const size_t bucket_type_count = (CHUNK_SIZE - sizeof(BlockList)) / bucket_type_size;
        const BlockList* block = buckets[i].address;

        size_t reserved = 0;
        size_t used = 0;
        while (block)
        {
            ++reserved;
            const char* ptr = static_cast<const char*>(block->chunk) + sizeof(BlockList);
            for (size_t j = 0; j < bucket_type_count; ++j)
            {
                const FreeList* record = reinterpret_cast<const FreeList*>(ptr);
                if (record->header.isValid())
                    ++used;
                ptr += bucket_type_size;
            }
            block = block->next;
        }
        total_reserved += reserved;
        total_used += used;
        total_mem += used * fsa_size;

        if (reserved)
        {
            std::cout << "  Buckets for " << fsa_size << "-bytes blocks reserved: " << reserved << '\n';
            std::cout << "    Blocks allocated: " << used << '\n';
        }
    }
    std::cout << "Total buckets reserved: " << total_reserved << '\n';
    std::cout << "Total allocated: " << total_used << " blocks in " << total_mem << " bytes\n";
    std::cout << "\n==========================================================\n";
}

void FSAllocator::dumpBlocks() const
{
    assert(state == State::Initialized);

    std::cout << "\n==========================================================\n";

    std::cout << "FSA:\nUser allocated blocks:\n";
    std::cout << "+------------------+------------------+------------------+\n";
    std::cout << "| Address of block |    Block Size    |  Requested size  |\n";
    std::cout << "+------------------+------------------+------------------+\n";
    for (BucketMap i = 0; i < BUCKETS_COUNT; ++i)
    {
        const BucketMap fsa_size = MIN_BYTES << i;
        const BucketMap bucket_type_size = fsa_size + ALIGN;
        const size_t bucket_type_count = (CHUNK_SIZE - sizeof(BlockList)) / bucket_type_size;
        const BlockList* block = buckets[i].address;

        while (block)
        {
            const char* ptr = static_cast<const char*>(block->chunk) + sizeof(BlockList);
            for (size_t j = 0; j < bucket_type_count; ++j)
            {
                const FreeList* record = reinterpret_cast<const FreeList*>(ptr);
                if (record->header.isValid())
                {
                    std::cout << "| " << std::setw(16) << std::left
                        << (void*)(reinterpret_cast<const char*>(record) + ALIGN)
                        << " | " << std::setw(16) << std::left << fsa_size
                        << " | " << std::setw(16) << std::left << record->header.getSize() << "|\n";
                }
                ptr += bucket_type_size;
            }
            block = block->next;
        }
    }
    std::cout << "+------------------+------------------+------------------+\n";
    std::cout << "\n==========================================================\n";
}

void CoaleseAllocator::dumpStat() const
{
    assert(state == State::Initialized);

    std::cout << "Coalese:\n";
    size_t total_reserved = 0;
    size_t total_used_mem = 0;
    size_t total_mem = 0;

    std::cout << "\n==========================================================\n";

    BlockList* block = block_list;
    while (block)
    {
        size_t block_used = 0;
        size_t block_free = 0;
        size_t block_used_mem = 0;
        size_t block_free_mem = 0;

        Header* record = block->getFirstRecord();
        size_t size = align(sizeof(Header));
        while (record && size < block->size)
        {
            size += (size_t)record->getSize();
            if (record->isBusy())
            {
                ++block_used;
                block_used_mem += (size_t)record->getSize();
            }
            else
            {
                ++block_free;
                block_free_mem += (size_t)record->getSize();
            }
            record = record->nextHeader();
        }
        ++total_reserved;
        total_used_mem += block_used_mem;
        total_mem += block->size;

        std::cout << "Block at Addr=" << (void*)block->chunk << ":\n";
        std::cout << "  Allocated Num=" << block_used << " parts of total Size=" << block_used_mem << '\n';
        std::cout << "  Free Num=" << block_free << " parts of total Size=" << block_free_mem << '\n';

        block = block->next;
    }
    std::cout << "Total blocks reserved: " << total_reserved << '\n';
    std::cout << "Total allocated: " << total_used_mem << " of " << total_mem << " bytes\n";
    std::cout << "\n==========================================================\n";
}

void CoaleseAllocator::dumpBlocks() const
{
    assert(state == State::Initialized);

    std::cout << "\n==========================================================\n";

    std::cout << "Coalese:\nUser allocated blocks:\n";
    std::cout << "+------------------+------------------+\n";
    std::cout << "|     Address      |        Size      |\n";
    std::cout << "+------------------+------------------+\n";
    BlockList* block = block_list;
    while (block)
    {
        Header* record = block->getFirstRecord();
        size_t header_size = align(sizeof(Header));
        size_t size = header_size;
        while (record && size < block->size)
        {
            size += (size_t)record->getSize();
            if (record->isBusy())
            {
                std::cout << "| " << std::setw(16) << std::left
                    << (void*)(reinterpret_cast<const char*>(record) + header_size)
                    << " | " << std::setw(16) << std::left << record->getSize() << " |\n";
            }
            record = record->nextHeader();
        }
        block = block->next;
    }
    std::cout << "+------------------+------------------+\n";
    std::cout << "\n==========================================================\n";
}
#endif

MemoryAllocator::MemoryAllocator()
{
    bool need_delete_on_throw = false;
    try
    {
        fs_al = new FSAllocator;
        need_delete_on_throw = true;
        co_al = new CoaleseAllocator;
    }
    catch (...)
    {
        if (need_delete_on_throw)
            delete fs_al;
        throw;
    }
}

MemoryAllocator::~MemoryAllocator()
{
    delete fs_al;
    delete co_al;
}

void MemoryAllocator::init()
{
    fs_al->init();
    co_al->init();
}

void MemoryAllocator::destroy()
{
    fs_al->destroy();
    co_al->destroy();
}

void* MemoryAllocator::alloc(size_t nbytes)
{
    void* result = nullptr;
    if (nbytes > BYTES_ON_SWITCH_ALLOCATOR)
        result = co_al->alloc(nbytes);
    else
        result = fs_al->alloc(nbytes);
    return result;
}

void MemoryAllocator::free(void* ptr)
{
    if (ptr)
    {
        char* p = static_cast<char*>(ptr) - ALIGN;
        size_t nbytes = *reinterpret_cast<size_t*>(p);
        nbytes &= 0x0000'ffff'ffff'ffff;
        if (nbytes > BYTES_ON_SWITCH_ALLOCATOR)
            co_al->free(ptr);
        else
            fs_al->free(ptr);
    }
}

void MemoryAllocator::dumpStat() const
{
    fs_al->dumpStat();
    co_al->dumpStat();
}

void MemoryAllocator::dumpBlocks() const
{
    fs_al->dumpBlocks();
    co_al->dumpBlocks();
}
