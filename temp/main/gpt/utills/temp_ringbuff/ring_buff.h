#pragma once

#include <cstddef>

template <typename T>
class RingBuffer
{
public:
    explicit RingBuffer(std::size_t size);
    ~RingBuffer();

    RingBuffer(const RingBuffer&) = delete;  // Disable copy
    RingBuffer& operator=(const RingBuffer&) = delete;

    RingBuffer(RingBuffer&& other) noexcept; // Allow move
    RingBuffer& operator=(RingBuffer&& other) noexcept;

    bool write(const T& data); // Write one object
    std::size_t write(const T* data, std::size_t len); // Write multiple objects
    bool read(T& data); // Read one object
    std::size_t read(T* data, std::size_t len); // Read multiple objects

    // Buffer status
    std::size_t available() const;
    std::size_t free_space() const;
    bool empty() const;
    bool full() const;

    // Remove all stored objects
    void clear();

private:
    T* buffer;
    std::size_t capacity;
    std::size_t write_idx;
    std::size_t read_idx;
    std::size_t count;
};

#include "ring_buff.tpp"