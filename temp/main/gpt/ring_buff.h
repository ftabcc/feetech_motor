#pragma once

#include <cstddef>
#include <cstdint>

class RingBuffer
{
private:
    uint8_t *buffer;
    size_t capacity;

    size_t write_idx = 0;
    size_t read_idx = 0;
    size_t count = 0;

public:
    explicit RingBuffer(size_t size);
    ~RingBuffer();

    size_t write(const uint8_t *data, size_t len);
    bool read(uint8_t &byte);

    size_t available() const;
    size_t free_space() const;

    bool empty() const;
    bool full() const;

    void clear();
};