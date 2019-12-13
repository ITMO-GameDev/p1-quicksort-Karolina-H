#ifndef SORT_H_INCLUDED
#define SORT_H_INCLUDED
#include <utility>

template<typename T, typename Cmp>
void insertion_sort(T* begin, T* end, Cmp comp)
{
	for (auto i = begin + 1; i < end; ++i)
	{
		auto value = *i;
		auto j = i;

		while (begin < j && comp(value, *(j - 1)))
		{
			*j = *(j - 1);
			--j;
		}
		*j = value;
	}
}

template<typename T, typename Cmp>
T* partition(T* first, T* last, Cmp comp)
{
	auto pivot = *last;
	auto p = first;

	for (auto i = first; i < last; i++)
	{
		if (!comp(pivot, *i))
		{
			std::iter_swap(i, p);
			++p;
		}
	}
	std::iter_swap(p, last);
	return p;
}

template<typename T, typename Cmp>
void sort(T* begin, T* end, Cmp comp)
{
	while (begin < end - 1)
	{
		if (end - begin < 16)
		{
			insertion_sort(begin, end, comp);
			break;
		}
		else
		{
			auto pivot = partition(begin, end - 1, comp);

			if (pivot - begin < end - pivot) {
				sort(begin, pivot, comp);
				begin = pivot + 1;
			}
			else {
				sort(pivot + 1, end, comp);
				end = pivot;
			}
		}
	}
}

#endif
