#pragma once

#include <cstdint>
#include <cstddef>

class RingBuffer
{
private:
    static constexpr size_t CAPACITY = 512;

    uint8_t buffer[CAPACITY]{};
    size_t write_idx = 0;
    size_t read_idx = 0;

public:
    size_t write(const uint8_t *data, size_t len);
    bool pop(uint8_t &byte);
    size_t available() const;
    bool empty() const;
    bool full() const;
    void clear();
};