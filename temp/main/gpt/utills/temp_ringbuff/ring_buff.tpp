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