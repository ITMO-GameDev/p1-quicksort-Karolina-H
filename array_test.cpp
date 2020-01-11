#include "gtest/gtest.h"
#include "array.h"

class ArrayTest : public ::testing::Test
{
protected:
    Array<int> array_default;
};

TEST_F(ArrayTest, DefaultConstructor)
{
    EXPECT_EQ(array_default.size(), size_t(0));

    auto it = array_default.iterator();
    EXPECT_FALSE(it.hasNext());
    EXPECT_FALSE(it.hasPrev());
}

TEST_F(ArrayTest, CapacityConstructor)
{
    Array<int> arr{ 10 };
    EXPECT_EQ(arr.size(), size_t(0));

    auto it = arr.iterator();
    EXPECT_FALSE(it.hasNext());
    EXPECT_FALSE(it.hasPrev());
}

TEST_F(ArrayTest, CopyConstructor)
{
    for (auto i = 0; i < 10; ++i)
        array_default.insert(i);

    Array<int> arr{ array_default };
    auto it = arr.iterator();

    EXPECT_EQ(arr.size(), size_t(10));
    for (auto i = 0; it.hasNext(); it.next(), ++i)
        EXPECT_EQ(it.get(), i);
}

TEST_F(ArrayTest, CopyAssignment)
{
    for (auto i = 0; i < 10; ++i)
        array_default.insert(i);

    Array<int> arr;
    arr = array_default;
    auto it = arr.iterator();

    EXPECT_EQ(array_default.size(), size_t(10));
    EXPECT_EQ(arr.size(), size_t(10));
    for (auto i = 0; it.hasNext(); it.next(), ++i)
        EXPECT_EQ(it.get(), i);
}

TEST_F(ArrayTest, Insert)
{
    for (auto i = 0; i < 10; ++i)
        array_default.insert(i);
    EXPECT_EQ(array_default.size(), size_t(10));

    auto it = array_default.iterator();
    for (auto i = 0; it.hasNext(); it.next(), ++i)
        EXPECT_EQ(it.get(), i);
}

TEST_F(ArrayTest, InsertAt)
{
    for (auto i = 0; i < 10; ++i)
        array_default.insert(size_t(i), i);
    EXPECT_EQ(array_default.size(), size_t(10));

    auto it = array_default.iterator();
    for (auto i = 0; i < 10; ++i, it.next())
        EXPECT_EQ(it.get(), i);

    for (auto i = 0; i < 10; ++i)
        array_default.insert(size_t(i), i);
    EXPECT_EQ(array_default.size(), size_t(20));

    it = array_default.iterator();
    for (auto i = 0; i < 10; ++i, it.next())
        EXPECT_EQ(it.get(), i);
    for (auto i = 0; i < 10; ++i, it.next())
        EXPECT_EQ(it.get(), i);
}

TEST_F(ArrayTest, Remove)
{
    array_default.insert(3);
    array_default.remove(0);
    EXPECT_EQ(array_default.size(), size_t(0));

    for (auto i = 0; i < 10; ++i)
        array_default.insert(i);
    array_default.remove(0);
    EXPECT_EQ(array_default.size(), size_t(9));
    array_default.remove(array_default.size() - 1);
    EXPECT_EQ(array_default.size(), size_t(8));
    array_default.remove((array_default.size() - 1) / 2);
    EXPECT_EQ(array_default.size(), size_t(7));
}

TEST_F(ArrayTest, Subscript)
{
    for (auto i = 0; i < 10; ++i)
        array_default.insert(i);

    for (auto i = 0; i < 10; ++i)
        EXPECT_EQ(array_default[size_t(i)], i);

    for (auto i = 0; i < 10; ++i)
        array_default[size_t(i)] = 10 + i;

    const auto copy = array_default;
    for (auto i = 0; i < 10; ++i)
        EXPECT_EQ(copy[size_t(i)], 10 + i);
}

TEST_F(ArrayTest, Iterators)
{
    auto it(array_default.iterator());
    for (auto i = 0; i < 10; ++i)
        array_default.insert(i);
    it = array_default.iterator();
    it.toIndex(5);
    Array<int>::ConstIterator cit;
    cit = it;
    EXPECT_EQ(it.get(), cit.get());
}
