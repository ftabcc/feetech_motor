#pragma once

#include <utility>

template <typename T>
RingBuffer<T>::RingBuffer(std::size_t size) : buffer(nullptr),capacity(size),write_idx(0),read_idx(0),count(0)
{
    if (capacity == 0)
    {return;}
    buffer = new T[capacity]{};
}

template <typename T>
RingBuffer<T>::~RingBuffer()
{
    delete[] buffer;
}

template <typename T>
RingBuffer<T>::RingBuffer(RingBuffer&& other) noexcept : buffer(other.buffer),capacity(other.capacity),write_idx(other.write_idx),read_idx(other.read_idx),count(other.count)
{
    other.buffer = nullptr;
    other.capacity = 0;
    other.write_idx = 0;
    other.read_idx = 0;
    other.count = 0;
}

template <typename T>
RingBuffer<T>& RingBuffer<T>::operator=(RingBuffer&& other) noexcept
{
    if (this == &other)
    {return *this;}
    delete[] buffer;

    buffer = other.buffer;
    capacity = other.capacity;
    write_idx = other.write_idx;
    read_idx = other.read_idx;
    count = other.count;

    other.buffer = nullptr;
    other.capacity = 0;
    other.write_idx = 0;
    other.read_idx = 0;
    other.count = 0;

    return *this;
}

template <typename T>
bool RingBuffer<T>::write(const T& data)
{
    if (buffer == nullptr || count >= capacity)
    {return false;}
    buffer[write_idx] = data;
    write_idx = (write_idx + 1) % capacity;
    count++;
    return true;
}


template <typename T>
std::size_t RingBuffer<T>::write(const T* data, std::size_t len)
{
    if (buffer == nullptr || data == nullptr || len == 0)
    {return 0;}
    std::size_t written = 0;
    while (written < len && count < capacity)
    {
        buffer[write_idx] = data[written];
        write_idx = (write_idx + 1) % capacity;
        count++;
        written++;
    }
    return written;
}

template <typename T>
bool RingBuffer<T>::read(T& data)
{
    if (buffer == nullptr || count == 0)
    {return false;}
    data = std::move(buffer[read_idx]);
    read_idx = (read_idx + 1) % capacity;
    count--;
    return true;
}

template <typename T>
std::size_t RingBuffer<T>::read(T* data, std::size_t len)
{
    if (buffer == nullptr || data == nullptr || len == 0)
    {return 0;}
    std::size_t read_count = 0;
    while (read_count < len && count > 0)
    {
        data[read_count] = std::move(buffer[read_idx]);
        read_idx = (read_idx + 1) % capacity;
        count--;
        read_count++;
    }
    return read_count;
}

template <typename T>
std::size_t RingBuffer<T>::find(const T& value, std::size_t start) const
{
    if (start >= count)
        return count;

    for (std::size_t i = start; i < count; ++i)
    {
        const std::size_t index = (read_idx + i) % capacity;

        if (buffer[index] == value)
            return i;
    }

    return count;
}

template <typename T>
std::size_t RingBuffer<T>::find(const T* pattern, std::size_t pattern_len, std::size_t start) const
{
    if (pattern_len == 0 || start >= count || pattern_len > count - start)
        return count;

    for (std::size_t i = start; i + pattern_len <= count; ++i) //sliding
    {
        bool matched = true;
        for (std::size_t j = 0; j < pattern_len; ++j) //find_head
        {
            const std::size_t index = (read_idx + i + j) % capacity;
            if (buffer[index] != pattern[j])
            {
                matched = false;
                break;
            }
        }
        if (matched)
            return i;
    }
    return count;
}

template <typename T>
bool RingBuffer<T>::peek(std::size_t offset, T& data) const
{
    if (offset >= count)
        return false;

    const std::size_t index = (read_idx + offset) % capacity;
    data = buffer[index];

    return true;
}

template <typename T>
std::size_t RingBuffer<T>::peek(T* data, std::size_t len, std::size_t offset) const
{
    if (data == nullptr || len == 0 || offset >= count || len > count - offset)
        return 0;

    for (std::size_t i = 0; i < len; ++i)
    {
        const std::size_t index = (read_idx + offset + i) % capacity;
        data[i] = buffer[index];
    }

    return len;
}

template <typename T>
bool RingBuffer<T>::get_write_ptr(T*& ptr, std::size_t requested_len, std::size_t& write_len)
{
    ptr = nullptr;
    write_len = 0;

    if (buffer == nullptr || count >= capacity || requested_len == 0)
        return false;

    const std::size_t free = capacity - count;

    std::size_t contiguous_len;

    if (write_idx < read_idx)
        contiguous_len = read_idx - write_idx;
    else
        contiguous_len = capacity - write_idx;

    write_len = (requested_len < contiguous_len) ? requested_len : contiguous_len;

    if (write_len == 0)
        return false;

    ptr = &buffer[write_idx];

    return true;
}

template <typename T>
std::size_t RingBuffer<T>::available() const
{
    return count;
}

template <typename T>
std::size_t RingBuffer<T>::free_space() const
{
    return capacity - count;
}

template <typename T>
bool RingBuffer<T>::empty() const
{
    return count == 0;
}

template <typename T>
bool RingBuffer<T>::full() const
{
    return count == capacity;
}

template <typename T>
void RingBuffer<T>::clear()
{
    write_idx = 0;
    read_idx = 0;
    count = 0;
}