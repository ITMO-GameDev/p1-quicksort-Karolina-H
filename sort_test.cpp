#include "gtest/gtest.h"
#include "sort.h"
#include <functional> // std::less
#include <algorithm> // std::is_sorted
#include <iterator> // std::size
#include <random>

TEST(SortTest, One)
{
    int arr[] = { 1 };
    sort(arr, arr + std::size(arr), std::less());
    EXPECT_TRUE(std::is_sorted(arr, arr + std::size(arr)));
}

TEST(SortTest, Two)
{
    {
        int arr[] = { 1, 0 };
        sort(arr, arr + std::size(arr), std::less());
        EXPECT_TRUE(std::is_sorted(arr, arr + std::size(arr)));
    }

    {
        int arr[] = { 0, 1 };
        sort(arr, arr + std::size(arr), std::less());
        EXPECT_TRUE(std::is_sorted(arr, arr + std::size(arr)));
    }
    
    {
        int arr[] = { 0, 0 };
        sort(arr, arr + std::size(arr), std::less());
        EXPECT_TRUE(std::is_sorted(arr, arr + std::size(arr)));
    }
}

TEST(SortTest, Three)
{
    {
        int arr[] = { 0, 0, 0 };
        sort(arr, arr + std::size(arr), std::less());
        EXPECT_TRUE(std::is_sorted(arr, arr + std::size(arr)));
    }

    {
        int arr[] = { 1, 0, 0 };
        sort(arr, arr + std::size(arr), std::less());
        EXPECT_TRUE(std::is_sorted(arr, arr + std::size(arr)));
    }

    {
        int arr[] = { 0, 1, 0 };
        sort(arr, arr + std::size(arr), std::less());
        EXPECT_TRUE(std::is_sorted(arr, arr + std::size(arr)));
    }

    {
        int arr[] = { 0, 0, 1 };
        sort(arr, arr + std::size(arr), std::less());
        EXPECT_TRUE(std::is_sorted(arr, arr + std::size(arr)));
    }
}

TEST(SortTest, Simple)
{
    int arr[] = { 10, 0, 1, 11, 2, 12, 3, 13, 4, 14, 5, 15, 6, 16, 7, 17, 8, 18, 9, 19 };
    sort(arr, arr + std::size(arr), std::less());
    EXPECT_TRUE(std::is_sorted(arr, arr + std::size(arr)));
}

TEST(SortTest, Sorted)
{
    int arr[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };
    sort(arr, arr + std::size(arr), std::less());
    EXPECT_TRUE(std::is_sorted(arr, arr + std::size(arr)));
}

TEST(SortTest, Reverse)
{
    int arr[] = { 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
    sort(arr, arr + std::size(arr), std::less());
    EXPECT_TRUE(std::is_sorted(arr, arr + std::size(arr)));
}

TEST(SortTest, Repeated)
{
}

TEST(SortTest, RandomPos)
{
    int arr[1000];
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 100);

    std::generate(arr, arr + std::size(arr), [&dist, &gen]() -> int
        { return dist(gen); });

    sort(arr, arr + std::size(arr), std::less());
    EXPECT_TRUE(std::is_sorted(arr, arr + std::size(arr)));
}

TEST(SortTest, RandomNeg)
{
    int arr[1000];
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(-100, 0);

    std::generate(arr, arr + std::size(arr), [&dist, &gen]() -> int
        { return dist(gen); });

    sort(arr, arr + std::size(arr), std::less());
    EXPECT_TRUE(std::is_sorted(arr, arr + std::size(arr)));
}

TEST(SortTest, RandomAny)
{
    int arr[1000];
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(-1000, 1000);

    std::generate(arr, arr + std::size(arr), [&dist, &gen]() -> int
        { return dist(gen); });

    sort(arr, arr + std::size(arr), std::less());
    EXPECT_TRUE(std::is_sorted(arr, arr + std::size(arr)));
}
