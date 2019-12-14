#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <cstddef> // std::size_t
#include <cassert> // std::assert()
#include <memory>  // std::addressof, std::move

template<typename T>
class Array final
{
    enum { default_storage_size = 8 };
public:
    Array()
        : Array(default_storage_size)
    {}

    explicit Array(size_t capacity)
        : data_ptr(), data_size(0), storage_size(capacity)
    {
        data_ptr = static_cast<T*>(::operator new[](storage_size * sizeof(T)));
    }

    Array(const Array& other)
        : Array(other.storage_size)
    {
        auto cur = data_ptr;
        for (auto ptr = other.data_ptr; ptr != other.data_ptr + other.data_size; ++ptr, ++cur)
            ::new(std::addressof(*cur)) T(*ptr);
        data_size = other.data_size;
    }

    ~Array()
    {
        delete_rep(data_ptr, data_size);
    }

    Array& operator=(const Array& other)
    {
        if (&other != this)
        {
            const auto other_size = other.data_size;
            if (other_size > storage_size)
            {
                auto new_ptr = static_cast<T*>(::operator new[](other_size * sizeof(T)));
                auto cur = new_ptr;
                try
                {
                    for (auto ptr = other.data_ptr; ptr != other.data_ptr + other_size; ++ptr, ++cur)
                        ::new(std::addressof(*cur)) T(*ptr);
                }
                catch (...)
                {
                    delete_rep(new_ptr, cur - new_ptr);
                    throw;
                }
                delete_rep(data_ptr, data_size);
                data_ptr = new_ptr;
                storage_size = other_size;
            }
            else if (other_size > data_size)
            {
                auto cur = data_ptr;
                for (auto ptr = other.data_ptr; ptr != other.data_ptr + data_size; ++ptr, ++cur)
                    *cur = *ptr;
                for (auto ptr = other.data_ptr + data_size; ptr != other.data_ptr + other_size; ++ptr, ++cur)
                    ::new(std::addressof(*cur)) T(*ptr);
            }
            else
            {
                auto cur = data_ptr;
                for (auto ptr = other.data_ptr; ptr != other.data_ptr + other_size; ++ptr, ++cur)
                    *cur = *ptr;
                while (cur != data_ptr + data_size)
                    cur->~T();
            }
            data_size = other_size;
        }
        return *this;
    }

    void insert(const T& value)
    {
        insert(data_size, value);
    }

    void insert(size_t index, const T& value)
    {
        assert(index <= data_size && "Intex out of bounds");

        if (data_size != storage_size)
        {
            if (index == data_size)
            {
                ::new(std::addressof(data_ptr[data_size])) T(value);
                ++data_size;
            }
            else
            {
                ::new(std::addressof(data_ptr[data_size])) T(std::move(data_ptr[data_size - 1]));
                ++data_size;

                std::move_backward(data_ptr + index, data_ptr + data_size - 2, data_ptr + data_size - 1);
                data_ptr[index] = value;
            }
        }
        else
        {
            const auto new_storage_size = storage_size * 2;
            auto new_ptr = static_cast<T*>(::operator new[](new_storage_size * sizeof(T)));
            auto cur = new_ptr;
            try
            {
                ::new(std::addressof(new_ptr[index])) T(value);

                for (auto ptr = data_ptr; ptr != data_ptr + index; ++ptr, ++cur)
                    ::new(std::addressof(*cur)) T(std::move_if_noexcept(*ptr));

                ++cur;

                for (auto ptr = data_ptr + index; ptr != data_ptr + data_size; ++ptr, ++cur)
                    ::new(std::addressof(*cur)) T(std::move_if_noexcept(*ptr));
            }
            catch (...)
            {
                if (cur == new_ptr)
                {
                    new_ptr[index].~T();
                    ::operator delete[](new_ptr);
                }
                else
                {
                    delete_rep(new_ptr, cur - new_ptr);
                }
                throw;
            }
            delete_rep(data_ptr, data_size);
            data_ptr = new_ptr;
            ++data_size;
            storage_size = new_storage_size;
        }
    }

    void remove(size_t index)
    {
        assert(index < data_size && "Intex out of bounds");
        if (index != data_size - 1)
            std::move(data_ptr + index + 1, data_ptr + data_size, data_ptr + index);
        --data_size;
        data_ptr[data_size].~T();
    }

    const T& operator [](size_t index) const
    {
        assert(index < data_size && "Intex out of bounds");
        return data_ptr[index];
    }

    T& operator [](size_t index)
    {
        assert(index < data_size && "Intex out of bounds");
        return data_ptr[index];
    }

    size_t size() const
    {
        return data_size;
    }

    class Iterator;
    class ConstIterator;

    Iterator iterator()
    {
        return Iterator(this);
    }

    ConstIterator iterator() const
    {
        return ConstIterator(this);
    }

private:
    void delete_rep(T* ptr, size_t nobj)
    {
        for (auto p = ptr; p != ptr + nobj; ++p)
            p->~T();
        ::operator delete[](ptr);
    }

    void swap_rep(Array& other)
    {
        std::swap(data_ptr, other.data_ptr);
        std::swap(data_size, other.data_size);
        std::swap(storage_size, other.storage_size);
    }

    T* data_ptr;
    size_t data_size;
    size_t storage_size;
};

template<typename T>
class Array<T>::Iterator
{
public:
    Iterator() = delete;

    const T& get() const
    {
        assert(ptr >= container->data_ptr
            && ptr < container->data_ptr + container->data_size
            && "Invalid iterator");
        return *ptr;
    }

    void set(const T& value)
    {
        assert(ptr >= container->data_ptr
            && ptr < container->data_ptr + container->data_size
            && "Invalid iterator");
        *ptr = value;
    }

    void insert(const T& value)
    {
        size_t index = ptr - container->data_ptr;
        container->insert(index, value);
        ptr = container->data_ptr + index;
    }

    void remove()
    {
        size_t index = ptr - container->data_ptr;
        container->remove(index);
    }

    void next()
    {
        ++ptr;
    }

    void prev()
    {
        --ptr;
    }

    void toIndex(size_t index)
    {
        ptr = container->data_ptr + index;
    }

    bool hasNext() const
    {
        return ptr < container->data_ptr + container->data_size - 1;
    }

    bool hasPrev() const
    {
        return ptr > container->data_ptr;
    }

private:
    explicit Iterator(Array* a)
        : container(a)
        , ptr(a->data_ptr)
    {}

private:
    friend class Array;

    Array* container;
    T* ptr;
};

template<typename T>
class Array<T>::ConstIterator
{
public:
    ConstIterator() = default;

    ConstIterator(const Iterator& it)
        : container(it.container)
        , ptr(it.ptr)
    {}

    const T& get() const
    {
        assert(ptr >= container->data_ptr
            && ptr < container->data_ptr + container->data_size
            && "Invalid iterator");
        return *ptr;
    }

    void next()
    {
        ++ptr;
    }

    void prev()
    {
        --ptr;
    }

    void toIndex(size_t index)
    {
        ptr = container->data_ptr + index;
    }

    bool hasNext() const
    {
        return ptr < container->data_ptr + container->data_size - 1;
    }

    bool hasPrev() const
    {
        return ptr > container->data_ptr;
    }

private:
    explicit ConstIterator(const Array* a)
        : container(a)
        , ptr(a->data_ptr)
    {}

private:
    friend class Array;

    const Array* container = nullptr;
    const T* ptr = nullptr;
};

#endif // !ARRAY_H_INCLUDED
