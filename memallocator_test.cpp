#include "gtest/gtest.h"
#include "memallocator.h"

class MemoryAllocatorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        allocator.init();
    }

    void TearDown() override
    {
        allocator.free(ptr);
    }

    MemoryAllocator allocator;
    void* ptr = nullptr;
};

TEST_F(MemoryAllocatorTest, Alloc0)
{
    ptr = allocator.alloc(0);
    ASSERT_EQ(ptr, nullptr);
}

TEST_F(MemoryAllocatorTest, Alloc16)
{
    ptr = allocator.alloc(16);
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ((uintptr_t)ptr % 8, 0);
}

TEST_F(MemoryAllocatorTest, Alloc32)
{
    ptr = allocator.alloc(32);
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ((uintptr_t)ptr % 8, 0);
}

TEST_F(MemoryAllocatorTest, Alloc64)
{
    ptr = allocator.alloc(64);
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ((uintptr_t)ptr % 8, 0);
}

TEST_F(MemoryAllocatorTest, Alloc128)
{
    ptr = allocator.alloc(128);
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ((uintptr_t)ptr % 8, 0);
}

TEST_F(MemoryAllocatorTest, Alloc256)
{
    ptr = allocator.alloc(256);
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ((uintptr_t)ptr % 8, 0);
}

TEST_F(MemoryAllocatorTest, Alloc512)
{
    ptr = allocator.alloc(512);
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ((uintptr_t)ptr % 8, 0);
}

TEST_F(MemoryAllocatorTest, AllocBig)
{
    ptr = allocator.alloc(1000 * sizeof(int));
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ((uintptr_t)ptr % 8, 0);
}

TEST_F(MemoryAllocatorTest, AllocTooBig)
{
    ptr = allocator.alloc(10 * 1024 * 1024 + 1);
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ((uintptr_t)ptr % 8, 0);
}
