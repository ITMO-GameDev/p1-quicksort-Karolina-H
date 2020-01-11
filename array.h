#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <cstddef> // std::size_t
#include <cassert> // std::assert()
#include <memory>  // std::addressof, std::move

// класс динамического массива
template<typename T>
class Array final
{
    enum { default_storage_size = 8 };
public:
    // конструктор по умолчанию
    Array()
        : Array(default_storage_size)
    {}

    // конструктор, резервирующий память для capacity элементов
    explicit Array(size_t capacity)
        : data_ptr(), data_size(0), storage_size(capacity)
    { // резервируем память для capacity элементов
        data_ptr = static_cast<T*>(::operator new[](storage_size * sizeof(T)));
    }

    // конструктор копирования
    Array(const Array& other)
        : Array(other.storage_size) // делегируем резервирование памяти
    {
        auto cur = data_ptr;
        // конструируем элементы в зарезервированой памяти
        for (auto ptr = other.data_ptr; ptr != other.data_ptr + other.data_size; ++ptr, ++cur)
            ::new(std::addressof(*cur)) T(*ptr);
        data_size = other.data_size;
    }

    // деструктор
    ~Array()
    {
        delete_rep(data_ptr, data_size);
    }

    // оператор пртисваивания
    Array& operator=(const Array& other)
    {
        if (&other != this)
        {
            const auto other_size = other.data_size;
            if (other_size > storage_size)
            { // если зарезервировано памяти меньше, чем размер other, выделяем память
                auto new_ptr = static_cast<T*>(::operator new[](other_size * sizeof(T)));
                auto cur = new_ptr;
                try
                { // конструируем в новой памяти элементы - копии из other
                    for (auto ptr = other.data_ptr; ptr != other.data_ptr + other_size; ++ptr, ++cur)
                        ::new(std::addressof(*cur)) T(*ptr);
                }
                catch (...)
                { // если при создании элементов выброшено исключение, уничтожаем вновь созданные
                  // элементы и освобождаем выделенную память
                    delete_rep(new_ptr, cur - new_ptr);
                    throw; // пробрасываем исключение дальше
                }
                delete_rep(data_ptr, data_size); // удаляем старые данные
                data_ptr = new_ptr; // устанавливаем новые
                storage_size = other_size;
            }
            else if (other_size > data_size) // памяти хватает, но новых элементов больше
            {
                auto cur = data_ptr;
                // копируем первые data_size элементы из other
                for (auto ptr = other.data_ptr; ptr != other.data_ptr + data_size; ++ptr, ++cur)
                    *cur = *ptr;
                // конструируем в this оставшиеся из other
                for (auto ptr = other.data_ptr + data_size; ptr != other.data_ptr + other_size; ++ptr, ++cur)
                    ::new(std::addressof(*cur)) T(*ptr);
            }
            else
            { // в other меньше элементов
                auto cur = data_ptr;
                // копируем элементы из other
                for (auto ptr = other.data_ptr; ptr != other.data_ptr + other_size; ++ptr, ++cur)
                    *cur = *ptr;
                // уничтожаем оставшиеся в this
                while (cur != data_ptr + data_size)
                    cur->~T();
            }
            data_size = other_size;
        }
        return *this;
    }

    // вставляет value в конец
    void insert(const T& value)
    {
        insert(data_size, value);
    }

    // вставляет value в позицию index
    void insert(size_t index, const T& value)
    {
        assert(index <= data_size && "Intex out of bounds");

        if (data_size != storage_size) // если зарезервированой памяти хватает
        {
            if (index == data_size)
            { // создаем элемент в конеце (в позиции, следующей за последним)
                ::new(std::addressof(data_ptr[data_size])) T(value);
                ++data_size;
            }
            else
            {
                // создаем последний элемент в позиции, следующей за последним
                ::new(std::addressof(data_ptr[data_size])) T(std::move(data_ptr[data_size - 1]));
                ++data_size;

                // перемещаем последние data_size - index - 1 элемент на 1 позицию вправо
                std::move_backward(data_ptr + index, data_ptr + data_size - 2, data_ptr + data_size - 1);
                // копируем value в позицию index
                data_ptr[index] = value;
            }
        }
        else
        {   // памяти не достаточно
            const auto new_storage_size = storage_size * 2; // удваиваем резерв
            // выделяем память под новый резерв
            auto new_ptr = static_cast<T*>(::operator new[](new_storage_size * sizeof(T)));
            auto cur = new_ptr;
            try
            {
                // создаем value в позиции index
                ::new(std::addressof(new_ptr[index])) T(value);

                // создаем первые index элементов в новой памяти
                for (auto ptr = data_ptr; ptr != data_ptr + index; ++ptr, ++cur)
                    ::new(std::addressof(*cur)) T(std::move_if_noexcept(*ptr));

                ++cur; // уже скопирован
                // создаем оставшиеся элементы в новой памяти
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

    // удаляет элемент в позиции index
    void remove(size_t index)
    {
        assert(index < data_size && "Intex out of bounds");
        if (index != data_size - 1) // если не последний элемент
            // сдвигаем влево на 1 все элементы правее index
            std::move(data_ptr + index + 1, data_ptr + data_size, data_ptr + index);
        --data_size;
        data_ptr[data_size].~T(); // уничтожаем последний элемент
    }

    // индексация
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

    class Iterator; // класс итератора
    class ConstIterator; // класс константного итератора

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
