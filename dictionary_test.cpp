#include "gtest/gtest.h"
#include "dictionary.h"

TEST(DictionaryTest, DefaultConstruct)
{
    Dictionary<int, int> d;

    EXPECT_EQ(d.size(), size_t(0));
}

TEST(DictionaryTest, CopyConstruct)
{
    Dictionary<int, int> d;
    d.put(1, 1);
    d.put(2, 2);
    d.put(3, 3);

    auto d_copy = d;
    EXPECT_EQ(d_copy.size(), d.size());
}

TEST(DictionaryTest, CopyAssignment)
{
    Dictionary<int, int> d;
    d.put(1, 1);
    d.put(2, 2);
    d.put(3, 3);

    decltype(d) d_copy;
    d_copy[1] = -1;
    d_copy = d;
    EXPECT_EQ(d_copy.size(), d.size());
}

TEST(DictionaryTest, Put)
{
    Dictionary<int, int> d;

    d.put(1, 1);
    EXPECT_EQ(d.size(), size_t(1));
    d.put(2, 1);
    EXPECT_EQ(d.size(), size_t(2));
    d.put(2, 2);
    EXPECT_EQ(d.size(), size_t(2));
    d.put(3, 3);
    EXPECT_EQ(d.size(), size_t(3));
    EXPECT_EQ(d[1], 1);
    EXPECT_EQ(d[2], 2);
    EXPECT_EQ(d[3], 3);
    EXPECT_EQ(static_cast<const decltype(d)&>(d)[0], int());
    EXPECT_EQ(static_cast<const decltype(d)&>(d)[100], int());
}

TEST(DictionaryTest, Remove)
{
    Dictionary<int, int> d;

    d.put(1, 1);
    d.put(2, 2);
    d.put(3, 3);

    d.remove(2);
    EXPECT_EQ(d.size(), size_t(2));
    EXPECT_EQ(d[1], 1);
    EXPECT_FALSE(d.contains(2));
    EXPECT_EQ(d[3], 3);
    EXPECT_EQ(static_cast<const decltype(d)&>(d)[2], int());

    d.remove(1);
    EXPECT_EQ(d.size(), size_t(1));
    EXPECT_EQ(static_cast<const decltype(d)&>(d)[1], int());
    EXPECT_FALSE(d.contains(1));
    EXPECT_EQ(d[3], 3);
    EXPECT_EQ(static_cast<const decltype(d)&>(d)[2], int());

    d.remove(3);
    EXPECT_EQ(d.size(), size_t(0));
    EXPECT_EQ(static_cast<const decltype(d)&>(d)[3], int());
    EXPECT_FALSE(d.contains(3));
}

TEST(DictionaryTest, Subscript)
{
    Dictionary<int, std::string> dict;
    std::string strings[] = {
        "zero",
        "one",
        "two"
    };

    for (int i = 0; i < sizeof(strings) / sizeof(strings[0]); ++i)
    {
        EXPECT_FALSE(dict.contains(i));
        dict[i] = strings[i];
        EXPECT_TRUE(dict.contains(i));
        EXPECT_EQ(dict[i], strings[i]);
    }
}

TEST(DictionaryTest, Iterator)
{
    Dictionary<std::string, int> npc;
    struct test_st
    {
        std::string s;
        int v;
    } const st[] = {
        {"health", 10},
        {"armor", 20},
        {"ammo", 5}
    };

    for (const auto& x : st)
        npc.put(x.s, x.v);

    auto it = npc.iterator();
    int count = 0;
    while (it.hasNext())
    {
        it.set(count);
        ++count;
        it.next();
    }
    it.set(count);
    ++count;
    EXPECT_EQ(count, sizeof(st) / sizeof(st[0]));
    EXPECT_FALSE(it.hasNext());
    EXPECT_TRUE(it.hasPrev());
    
    while (it.hasPrev())
    {
        --count;
        EXPECT_EQ(it.get(), count);
        it.prev();
    }
    --count;
    EXPECT_EQ(it.get(), count);
    EXPECT_EQ(count, 0);
    EXPECT_FALSE(it.hasPrev());
    EXPECT_TRUE(it.hasNext());
}
